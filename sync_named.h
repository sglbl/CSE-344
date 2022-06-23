#ifndef SYNCUNNAMED_H_
#define SYNCUNNAMED_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define NUMBER_OF_CHEFS 6
#define NUMBER_OF_INGREDIENTS 4
#define NUMBER_OF_SEMAPHORES 18

typedef struct SharedMemory {
    int numberOfLines;
    int isMilk, isFlour, isWalnut, isSugar;
    char ingredients[2];
} SharedMemory;

/* 
    chef0 has an endless supply of milk and flour but lacks walnuts and sugar, 
    chef1 has an endless supply of milk and sugar but lacks flour and walnuts, 
    chef2 has an endless supply of milk and walnuts but lacks sugar and flour, 
    chef3 has an endless supply of sugar and walnuts but lacks milk and flour, 
    chef4 has an endless supply of sugar and flour but lacks milk and walnuts, 
    chef5 has an endless supply of flour and walnuts but lacks sugar and milk. 
*/

/* Print error message with error code and exit */
void errorAndExit(char *errorMessage);

/* Initialize signal handler's prerequirements */
void signalHandlerInitializer();

/* Signal handler */
void sg_signalHandler(int signalNumber);

/* Store the array */
void arrayStorer(char* dataFilePath, char *name);

/* Handle the problem */
void problemHandler();

/* Chef (child) process */
int chef(int chefNumber);

/* Wholesaler (parent) process */
void wholesalerProcess(pid_t pidsFromFork[]);

/* Converts initial of ingredient to string */
char *stringConverter(char character);

char *sg_itoa(int number);

/* Pushers for ingredients */
void pusherMilk();

void pusherFlour();

void pusherWalnut();

void pusherSugar();

#endif