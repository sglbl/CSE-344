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
    sem_t milkSignal, flourSignal, walnutSignal, sugarSignal;  
    sem_t wholesaler;
    int isMilk, isFlour, isWalnut, isSugar;
    char ingredients[2];
} SharedMemory;

void errorAndExit(char *errorMessage);

void arrayStorer(char* dataFilePath);

void problemHandler();

void chef(int chefNumber);

void wholesalerProcess();

void pusherMilk();
void pusherFlour();
void pusherWalnut();
void pusherSugar();


#endif