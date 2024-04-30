//---------------------------------------------------------------------
// Assignment : PA-03 UDP Single-Threaded Server
// Date       :
// Author     : Max Adams & Aidan Herring
// File Name  : procurement.c
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

#include <sys/stat.h>
#include <sys/wait.h>

#include "wrappers.h"
#include "message.h"

#define MAXFACTORIES 20

typedef struct sockaddr SA;

/*-------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int numFactories,                  // Total Number of Factory Threads
        activeFactories,               // How many are still alive and manufacturing parts
        iters[MAXFACTORIES + 1] = {0}, // num Iterations completed by each Factory
        partsMade[MAXFACTORIES + 1] = {0}, totalItems = 0;

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
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("socket error");
        exit(1);
    }

    // Prepare the server's socket address structure

    // missing code goes here
    struct sockaddr_in srvrSkt;
    memset(&srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP, &srvrSkt.sin_addr) <= 0)
    {
        perror("inet_pton error");
        exit(1);
    }

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

    // Monitor all Active Factory Lines & Collect Production Reports
    while (activeFactories > 0) // wait for messages from sub-factories
    {

        // missing code goes here
        // Receive a message from a sub-factory
        msgBuf msg;
        if (recvfrom(sd, &msg, sizeof(msg), 0, (SA *)&srvrSkt, &len) < 0)
        {
            perror("recvfrom error");
            exit(1);
        }

        // Inspect the incoming message

        // missing code goes here
        switch (ntohl(msg.purpose))
        {
        case PRODUCTION_MSG:
            iters[ntohl(msg.facID)]++;
            partsMade[ntohl(msg.facID)] += ntohl(msg.partsMade);
            printf("PROCUREMENT: Factory #%d   produced %d   parts in %d   milliSecs\n",
                   ntohl(msg.facID), ntohl(msg.partsMade), ntohl(msg.duration));
            break;
        case COMPLETION_MSG:
            activeFactories--;
            printf("PROCUREMENT: Factory #%d         COMPLETED its task\n", ntohl(msg.facID));
            break;
        default:
            printf("Invalid message purpose: %d\n", ntohl(msg.purpose));
            break;
        }
    }

    // Print the summary report
    totalItems = 0;
    printf("\n\n****** PROCUREMENT Summary Report ******\n");

    // missing code goes here
    // Print the summary for each factory
    for (int i = 1; i <= numFactories; i++)
    {
        printf("Factory #  %d made a total of %d  parts in    %d iterations\n", i, partsMade[i], iters[i]);
        totalItems += partsMade[i];
    }

    printf("==============================\n");

    // missing code goes here
    // Print the total items made and the original order size
    printf("Grand total parts made =   %d   vs  order size of   %d\n", totalItems, orderSize);

    printf("\n>>> Procurement Terminated\n");

    // missing code goes here
    // Close the socket
    close(sd);

    return 0;
}