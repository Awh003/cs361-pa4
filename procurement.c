#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "wrappers.h"
#include "message.h"

#define MAXFACTORIES 20

typedef struct sockaddr SA;
int activeFactories;
int partsMade[MAXFACTORIES + 1];
int iters[MAXFACTORIES + 1]; // Define iters array
int sd;
struct sockaddr_in srvrSkt;

// The function that each procurement thread will run
void *procurement_thread_func(void *arg)
{
    // Extract the factory ID from arg
    int factoryID = *(int *)arg;

    // Loop until a Completion message has been received from the factory
    while (1)
    {
        // Receive a message from the factory
        msgBuf msg;
        socklen_t len = sizeof(srvrSkt);
        if (recvfrom(sd, &msg, sizeof(msg), 0, (SA *)&srvrSkt, &len) < 0)
        {
            perror("recvfrom error");
            exit(1);
        }

        switch (ntohl(msg.purpose))
        {
        case PRODUCTION_MSG:
            // If the message is a Production message, update the total number of parts received
            partsMade[ntohl(msg.facID)] += ntohl(msg.partsMade);
            printf("PROCUREMENT: Factory #%d   produced %d   parts in %d   milliSecs\n",
                   ntohl(msg.facID), ntohl(msg.partsMade), ntohl(msg.duration));
            // Increment the iteration count for this factory
            iters[ntohl(msg.facID)]++;
            break;
        case COMPLETION_MSG:
            // If the message is a Completion message, break the loop
            activeFactories--;
            printf("PROCUREMENT: Factory #%d         COMPLETED its task\n", ntohl(msg.facID));
            return NULL;
        default:
            printf("Invalid message purpose: %d\n", ntohl(msg.purpose));
            break;
        }
    }
}

/*-------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int numFactories, // Total Number of Factory Threads               // How many are still alive and manufacturing parts
        totalItems = 0;

    char *myName = "Max Adams & Aidan Herring";
    printf("\nPROCUREMENT: Started. Developed by %s\n\n", myName);
    fflush(stdout);

    if (argc < 4)
    {
        printf("PROCUREMENT Usage: %s  <order_size> <FactoryServerIP>  <port>\n", argv[0]);
        exit(-1);
    }

    unsigned orderSize = atoi(argv[1]);
    char *serverIP = argv[2];
    unsigned short port = (unsigned short)atoi(argv[3]);

    printf("Attempting Factory server at '%s' : %d\n", serverIP, port); // Move this line here
    fflush(stdout);

    /* Set up local and remote sockets */

    // missing code goes here
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("socket error");
        exit(1);
    }

    // Prepare the server's socket address structure

    // missing code goes here
    memset(&srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP, &srvrSkt.sin_addr) <= 0)
    {
        perror("inet_pton error");
        exit(1);
    }

    // Record the start time
    struct timeval start_time, end_time;
    double elapsedTime;
    gettimeofday(&start_time, NULL);

    // Send the initial request to the Factory Server
    msgBuf msg1;

    // missing code goes here
    msg1.purpose = htonl(REQUEST_MSG);
    msg1.orderSize = htonl(orderSize);
    if (sendto(sd, &msg1, sizeof(msg1), 0, (SA *)&srvrSkt, sizeof(srvrSkt)) < 0)
    {
        perror("sendto error");
        exit(1);
    }

    printf("\nPROCUREMENT Sent this message to the FACTORY server: ");
    printMsg(&msg1);
    puts("");

    /* Now, wait for oreder confirmation from the Factory server */
    msgBuf msg2;
    printf("\nPROCUREMENT is now waiting for order confirmation ...\n");

    // missing code goes here
    socklen_t len = sizeof(srvrSkt);
    if (recvfrom(sd, &msg2, sizeof(msg2), 0, (SA *)&srvrSkt, &len) < 0)
    {
        perror("recvfrom error");
        exit(1);
    }

    printf("PROCUREMENT received this from the FACTORY server: ");
    printMsg(&msg2);
    puts("\n");

    // missing code goes here
    numFactories = activeFactories = ntohl(msg2.numFac);

    // Initialize the iters array
    memset(iters, 0, sizeof(iters));

    // Create a thread for each factory
    pthread_t procurement_threads[numFactories];
    for (int i = 0; i < numFactories; i++)
    {
        // Prepare the argument for the thread (factory ID)
        int *arg = malloc(sizeof(int));
        *arg = i + 1;

        // Create the thread
        pthread_create(&procurement_threads[i], NULL, procurement_thread_func, arg);
    }

    // Wait for all procurement threads to terminate
    for (int i = 0; i < numFactories; i++)
    {
        pthread_join(procurement_threads[i], NULL);
    }

    // Record the end time
    gettimeofday(&end_time, NULL);

    // Calculate the elapsed time in milliseconds
    elapsedTime = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0;    // seconds to milliseconds
    elapsedTime += (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0; // microseconds to milliseconds

    // Print the summary report
    totalItems = 0;
    printf("\n\n****** PROCUREMENT Summary Report ******\n");
    printf("    Sub-Factory      Parts Made      Iterations\n");
    for (int i = 1; i <= numFactories; i++)
    {
        printf("              %d             %d               %d\n", i, partsMade[i], iters[i]);
        totalItems += partsMade[i];
    }
    printf("===================================================\n");
    printf("Grand total parts made   =   %d   vs  order size of   %d\n", totalItems, orderSize);
    printf("Order-to-Completion time =  %.1f milliSeconds\n", elapsedTime);

    printf("\n>>> Procurement Terminated\n");

    // Close the socket
    close(sd);

    return 0;
}