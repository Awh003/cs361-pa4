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

#include <sys/stat.h>
#include <sys/wait.h>

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

void subFactory(void *args);

void factLog(char *str)
{
    printf("%s", str);
    fflush(stdout);
}

/*-------------------------------------------------------*/

// Global Variable for Future Thread to Share
int remainsToMake, // Must be protected by a Mutex
    actuallyMade;  // Actually manufactured items

int sd; // Server socket descriptor
struct sockaddr_in
    srvrSkt, /* the address of this server   */
    clntSkt; /* remote client's socket       */
char ipStr[IPSTRLEN];

// For thread arguments
struct ThreadArgs {
    int facID; // Factory ID
    int myCap; // Capacity of the factory
    int myDur; // Duration of each cycle of the factory
};

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
    if (sigactionWrapper(SIGINT, goodbye) == SIG_ERR) {
        perror("Can't set signal handler for SIGINT");
        exit(1);
    }
    if (sigactionWrapper(SIGTERM, goodbye) == SIG_ERR) {
        perror("Can't set signal handler for SIGTERM");
        exit(1);
    }

    printf("\nThis is the FACTORY server developed by %s\n", myName);
    
    if (argc < 4)
    {
        printf("FACTORY Usage: %s [socketNumber] [numThreads] [port]\n", argv[0]);
        exit(-1);
    }

    N = atoi(argv[1]);
    ipStr = argv[2];
    unsigned short port = (unsigned short)atoi(argv[3]);

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
    srvrSkt.sin_addr.s_addr = inet_addr(ipStr);
    srvrSkt.sin_port = htons(port);

    if (bind(sd, (SA *)&srvrSkt, sizeof(srvrSkt)) < 0)
    {
        perror("bind error");
        exit(1);
    }

    inet_ntop(AF_INET, (void *)&srvrSkt.sin_addr.s_addr, ipStr, IPSTRLEN);
    printf("\nBound socket %d to IP %s Port %d\n", sd, ipStr, ntohs(srvrSkt.sin_port));

    int numThreads = 0;
    pthread_t threads[N];
    int forever = 1;
    srandom(time(NULL));
    while (forever)
    {
        printf("\nFACTORY server waiting for Order Requests\n");

        // missing code goes here
        // Reeive a message from the client
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

        if (numThreads < N) {
            // Generate random values for factory thread
            struct ThreadArgs args;
            args.facID = numThreads + 1;
            args.myCap = (int)random() % (50 - 10 + 1) + 10;
            args.myDur = (int)random() % (1200 - 500 + 1) + 500;
            // Create thread
            if (pthread_create(&thread, NULL, subFactory, &args) != 0) {
                threads[numThreads] 
                perror("pthread_create error");
                exit(1)
            }
            numThreads++;
        }
    }

    return 0;
}
// int factoryID, int myCapacity, int myDuration
void subFactory(void *args)
{
    // Grab arguments
    int factoryID = args.facID;
    int myCapacity = args.myCap;
    int myDuration = args.myDur;

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
}