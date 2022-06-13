#ifndef SERVANT_H_
#define SERVANT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

static int portNoThatServantIsListeningOn = 59161;

typedef struct SgDateLinkedList{
    String date;
    SgLinkedList *transactions;
    struct SgDateLinkedList *next;
} SgDateLinkedList;

typedef struct SgCityLinkedList{
    String cityName;
    SgDateLinkedList *dateLL;
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
/* Adds date to linked list */
SgDateLinkedList *addDateToLinkedList(SgDateLinkedList *head, char *date);
/* Adds transaction to linked list */
SgLinkedList *addTransactionToLinkedList(SgLinkedList *head, char *transaction);
/* Adds city to linked list */
SgCityLinkedList *addCityToLinkedList(SgCityLinkedList *head, char *cityName);
/* Reads files of transcations */
void readFileOfTranscations(SgLinkedList *transactions, char *cityDirPath, char *date);
/* Prints linked list */
void printLinkedList(SgLinkedList *head);
/* Prints city list */
void printCityLinkedList(SgCityLinkedList *head);
/* Prints date list */
void printDateLinkedList(SgCityLinkedList *iter);
/* COnnects to server with socket*/
void servantTcpCommWithServer(SgCityLinkedList *cityList, char *ipv4Adress, int portNo, int head, int tail);
/* City queue parser ; eg: 1-9 will parse to head=1, tail=9 */
void cityQueueParser(char *citiesToHandle, int *head, int *tail);
/* Returns pid of current process */
int getPidWithProp();
/* Frees linked list */
// void freeLinkedList(SgLinkedList *head);

#endif