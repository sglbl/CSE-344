#ifndef SERVANT_H_
#define SERVANT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include "common.h"

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
SgLinkedList *addToLinkedList(SgLinkedList *head, char *filePath);
/* Frees linked list */
// void freeLinkedList(SgLinkedList *head);
/* Prints linked list */
void printLinkedList(SgLinkedList *head);
/* COnnects to server with socket*/
void connectToTheServer(char *ipv4Adress, int portNo, int head, int tail);
/* City queue parser ; eg: 1-9 will parse to head=1, tail=9 */
void cityQueueParser(char *citiesToHandle, int *head, int *tail);


#endif