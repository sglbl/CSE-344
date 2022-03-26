#ifndef SGPROCESSP_H_
#define SGPROCESSP_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#define TRUE 1
#define FALSE 0

//* Prints error and instruction about how to run app */
void printErrorAndExit();

void reader(int fileDescriptor, char *argv[]);

int spawn(char *argv[], char *buffer[], char* i);


/* Dividing argv[1] into different operations as 2D array / double pointer */
char** argDivider(char* arg, int *counter);



#endif