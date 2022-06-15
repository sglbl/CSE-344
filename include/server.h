#ifndef SERVER_H_
#define SERVER_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

/* Creates threads */
void createThreads(int portNo);
/* Thread routine */
void *threadJob(void *arg);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Stream socket communication with servant and client */
void *tcpComm();
/* Main thread forwards incoming connections to threads */
void handleIncomingConnection(int newServerSocketFd);

void addToQueue(int newFileDesc);

void addServantInfoToList(ServantSendingInfo receivedInfoFromServant);

int removeFromQueue();

void findCityCodeAndResponsibleServant(char *cityName, int *cityCode, int *responsibleServant);

#endif