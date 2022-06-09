#ifndef CLIENT_H_
#define CLIENT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

/* Opens files */
void openFiles(char *filePath1, char *filePath2, char *output, int fileDescs[3]);
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
void getRequests(char *buffer, int numberOfRequests, String *lineData);
/* Creates threads */
void createThreads(int portNo, char *ipv4, int numOfThreads, String *lineData);

#endif