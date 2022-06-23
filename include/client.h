#ifndef CLIENT_H_
#define CLIENT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

typedef struct InfoFromClientToServer {
    int threadId;
    String lineData;
} InfoFromClientToServer;

/* Client routine */
void *doClientJob(void *arg);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Barrier implementation */
void barrier();
/* Opens request file */
int openRequestFile(char *filePath);
/* Gets number of requests */
int getNumberOfRequests(int fileSize, int requestFd, char *buffer);
/* Gets requests in variable of lineData*/
void getRequests(char *buffer, String *lineData);
/* Creates threads */
void createThreads(String *lineData);
/* Tcp communication with server */
void clientTcpCommWithServer(char *data, int threadNo);

#endif