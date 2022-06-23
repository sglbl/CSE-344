#ifndef SERVER_H_
#define SERVER_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

#define SINGLE_CITY_HANDLING 1
#define MULTIPLE_CITIES_HANDLING 2

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
void handleIncomingConnectionOfClient(int newServerSocketFd);
void handleIncomingConnectionOfServant(int newServerSocketFd);
/* Adds fd to queue */
void addToQueue(int newFileDesc);
/* Adds servant info to list */
void addServantInfoToList(ServantSendingInfo receivedInfoFromServant);
/* Removes from queue */
int removeFromQueue();
/* Finds responsible servant */
void findResponsibleServant(char *cityName, int *responsibleServant);
/* Finds responsible servant and gets trancations */
int getTransactionCountFromServant(int singleCityHandle, int responsibleServant, char *estateType, char *beginDate, char *endDate, char *cityName);

void dateParser(char *date, int *dateArray);;

#endif