#ifndef SERVANT_H_
#define SERVANT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

typedef struct SgCityLinkedList{
    String cityName;
    SgLinkedList *transactions;
    struct SgCityLinkedList *next;
} SgCityLinkedList;

/* Servant routine */
void doServantJob();
/* Creates threads */
void createThreads(int portNo);
/* Thread routine */
void *threadJob(void *arg);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Barrier implementation */
void barrier();
/* Integer to string */
char *itoaForAscii(int number);
/* Adds value to linked list */
SgLinkedList *addTransactionToLinkedList(SgLinkedList *head, char *transaction);
/* Adds city to linked list */
SgCityLinkedList *addCityToLinkedList(SgCityLinkedList *head, char *cityName);
/* Prints linked list */
void printLinkedList(SgLinkedList *head);
/* Prints city list */
void printCityLinkedList(SgCityLinkedList *head);
/* COnnects to server with socket*/
void servantTcpCommWithServer(char *ipv4Adress, int portNo, int head, int tail);
/* City queue parser ; eg: 1-9 will parse to head=1, tail=9 */
void cityQueueParser(char *citiesToHandle, int *head, int *tail);
/* Frees linked list */
// void freeLinkedList(SgLinkedList *head);

#endif