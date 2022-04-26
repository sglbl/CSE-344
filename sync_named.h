#ifndef SYNCNAMED_H_
#define SYNCNAMED_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0
#define NUMBER_OF_CHEFS 6
#define NUMBER_OF_INGREDIENTS 4

typedef struct SharedMemory {
    int numberOfLines;
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