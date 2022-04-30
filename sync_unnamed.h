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

typedef struct SharedMemory {
    int numberOfLines;
    sem_t signalToChef[NUMBER_OF_CHEFS];
    sem_t accessing, milk, flour, walnut, sugar;
    sem_t walnutAndFlourSignal, walnutAndSugarSignal, walnutAndMilkSignal, milkAndSugarSignal, milkAndFlourSignal, flourAndSugarSignal;
    sem_t dessertPrepared;
    sem_t childReturned;
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

void errorAndExit(char *errorMessage);

void signalHandlerInitializer();

void sg_signalHandler(int signalNumber);

void arrayStorer(char* dataFilePath);

void problemHandler();

int chef(int chefNumber);

void wholesalerProcess(pid_t pidsFromFork[]);

int totalDessertNumberFinder();

char *stringConverter(char character);

void pusherMilk();

void pusherFlour();

void pusherWalnut();

void pusherSugar();


#endif