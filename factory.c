//---------------------------------------------------------------------
// Assignment : PA-04 Threads-UDP
// Date       :
// Author     : Max Adams & Aidan Herring
// File Name  : factory.c
//---------------------------------------------------------------------

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
#include <sys/time.h> // For timing functions

#include "wrappers.h"
#include "message.h"

#define MAXSTR 200
#define IPSTRLEN 50

typedef struct sockaddr SA;
typedef void Sigfunc(int);

int minimum(int a, int b)
{
    return (a <= b ? a : b);
}

void *subFactory(void *arg);

void factLog(char *str)
{
    printf("%s", str);
    fflush(stdout);
}

/*-------------------------------------------------------*/

// Global Variable for Future Thread to Shared
int remainsToMake, // Must be protected by a Mutex
    actuallyMade;  // Actually manufactured items

int sd; // Server socket descriptor
struct sockaddr_in
    srvrSkt, /* the address of this server   */
    clntSkt; /* remote client's socket       */
char ipStr[IPSTRLEN];

// Global variables to store the summary report
int partsMade[MAXFACTORIES + 1] = {0};
int iters[MAXFACTORIES + 1] = {0};
double elapsedTime = 0;

//------------------------------------------------------------
//  Handle Ctrl-C or KILL
//------------------------------------------------------------
void goodbye(int sig)
{
    /* Mission Accomplished */
    printf("\n### I (%d) have been nicely asked to TERMINATE. "
           "goodbye\n\n",
           getpid());

    // missing code goes here
    // Close the socket
    close(sd);
    exit(0);
}

/*-------------------------------------------------------*/
int main(int argc, char *argv[])
{
    char *myName = "Max Adams & Aidan Herring";
    unsigned short port = 50015; /* service port number  */
    int N = 1;                   /* Num threads serving the client */

    // Set up signal handlers
    if (sigactionWrapper(SIGINT, goodbye) == SIG_ERR)
    {
        perror("Can't set signal handler for SIGINT");
        exit(1);
    }
    if (sigactionWrapper(SIGTERM, goodbye) == SIG_ERR)
    {
        perror("Can't set signal handler for SIGTERM");
        exit(1);
    }

    printf("\nThis is the FACTORY server developed by %s\n", myName);
    switch (argc)
    {
    case 1:
        break; // use default port with a single factory thread

    case 2:
        N = atoi(argv[1]); // get from command line
        port = 50015;      // use this port by default
        break;

    case 3:
        N = atoi(argv[1]);    // get from command line
        port = atoi(argv[2]); // use port from command line
        break;

    default:
        printf("FACTORY Usage: %s [numThreads] [port]\n", argv[0]);
        exit(1);
    }

    // missing code goes here
    // Create a socket
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("Could NOT create socket");
        exit(1);
    }

    // Bind the socket to the server address
    memset((void *)&srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_addr.s_addr = htonl(INADDR_ANY);
    srvrSkt.sin_port = htons(port);

    if (bind(sd, (SA *)&srvrSkt, sizeof(srvrSkt)) < 0)
    {
        perror("bind error");
        exit(1);
    }

    inet_ntop(AF_INET, (void *)&srvrSkt.sin_addr.s_addr, ipStr, IPSTRLEN);
    printf("\nBound socket %d to IP %s Port %d\n", sd, ipStr, ntohs(srvrSkt.sin_port));

    int forever = 1;
    while (forever)
    {
        printf("\nFACTORY server waiting for Order Requests\n");

        // missing code goes here
        // Receive a message from the client
        socklen_t len = sizeof(clntSkt);
        msgBuf msg1;
        if (recvfrom(sd, &msg1, sizeof(msg1), 0, (SA *)&clntSkt, &len) < 0)
        {
            perror("recvfrom error");
            exit(1);
        }

        // Convert the client's IP and port to human-readable form
        char clntIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clntSkt.sin_addr), clntIP, INET_ADDRSTRLEN);
        unsigned short clntPort = ntohs(clntSkt.sin_port);

        printf("\n\nFACTORY server received: ");
        printMsg(&msg1);
        printf("\nFrom IP %s Port %d\n", clntIP, clntPort);
        puts("");

        // Prepare the order confirmation message
        msg1.purpose = htonl(ORDR_CONFIRM);
        msg1.numFac = htonl(N);
        remainsToMake = ntohl(msg1.orderSize);

        remainsToMake = ntohl(msg1.orderSize);

        // missing code goes here
        // Send a message to the client
        if (sendto(sd, &msg1, sizeof(msg1), 0, (SA *)&clntSkt, len) < 0)
        {
            perror("sendto error");
            exit(1);
        }

        printf("\nFACTORY sent this Order Confirmation to the client ");
        printMsg(&msg1);
        puts("");

        struct timeval start, end;
        gettimeofday(&start, NULL);

        // Create a thread for each factory
        srand(time(NULL));
        pthread_t factory_threads[N];
        int *args[N];
        for (int i = 0; i < N; i++)
        {
            // Prepare the arguments for the thread (factory ID, capacity, and duration)
            args[i] = malloc(sizeof(int) * 3);
            args[i][0] = i + 1;
            args[i][1] = rand() % 50 + 1;   // replace with actual capacity
            args[i][2] = rand() % 1000 + 1; // replace with actual duration

            // Create the thread
            pthread_create(&factory_threads[i], NULL, subFactory, args[i]);
        }

        // Wait for all factory threads to terminate
        for (int i = 0; i < N; i++)
        {
            pthread_join(factory_threads[i], NULL);
            free(args[i]);
        }

        gettimeofday(&end, NULL);
        elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
        elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms

        // Print the summary report
        printf("\n\n****** FACTORY Server Summary Report ******\n");
        printf("    Sub-Factory      Parts Made      Iterations\n");
        int totalPartsMade = 0;
        int totalIterations = 0;
        for (int i = 1; i <= N; i++)
        {
            printf("              %d             %d               %d\n", i, partsMade[i], iters[i]);
            totalPartsMade += partsMade[i];
            totalIterations += iters[i];
        }
        printf("====================================================\n");
        printf("Grand total parts made   =   %d   vs  order size of   %d\n", totalPartsMade, remainsToMake); // Use remainsToMake for order size
        printf("Order-to-Completion time =  %.1f milliSeconds\n", elapsedTime);
    }

    return 0;
}

void *subFactory(void *arg)
{
    int *args = (int *)arg;
    int factoryID = args[0];
    int myCapacity = args[1];
    int myDuration = args[2];

    char strBuff[MAXSTR]; // print buffer
    int partsImade = 0, myIterations = 0;
    msgBuf msg;

    while (1)
    {
        // See if there are still any parts to manufacture
        if (remainsToMake <= 0)
            break; // Not anymore, exit the loop

        // Decide how many parts to make
        int partsToMake = minimum(myCapacity, remainsToMake);

        // Print the number of parts the factory is going to make
        printf("Factory #  %d: Going to make    %d parts in  %d mSec\n", factoryID, partsToMake, myDuration);
        fflush(stdout);

        remainsToMake -= partsToMake;

        // Sleep for the specified duration
        usleep(myDuration * 1000);

        // Update the number of parts made
        partsImade += partsToMake;
        myIterations++;

        // Send a Production Message to Supervisor
        msg.purpose = htonl(PRODUCTION_MSG);
        msg.facID = htonl(factoryID);
        msg.capacity = htonl(myCapacity);
        msg.partsMade = htonl(partsToMake);
        msg.duration = htonl(myDuration);
        if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, sizeof(clntSkt)) < 0)
        {
            perror("sendto error");
            exit(1);
        }
    }

    // Send a Completion Message to Supervisor
    msg.purpose = htonl(COMPLETION_MSG);
    msg.facID = htonl(factoryID);
    if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, sizeof(clntSkt)) < 0)
    {
        perror("sendto error");
        exit(1);
    }

    snprintf(strBuff, MAXSTR, ">>> Factory #  %-3d: Terminating after making total of %-5d   parts in %-4d    iterations\n", factoryID, partsImade, myIterations);
    factLog(strBuff);

    // Update the summary report
    partsMade[factoryID] += partsImade;
    iters[factoryID] += myIterations;

    return NULL;
}