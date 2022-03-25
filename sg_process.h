#ifndef SGPROCESS_H_
#define SGPROCESS_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

//* Prints error and instruction about how to run app */
void printErrorAndExit();

void reader(int fileDescriptor, char *argv[]);

int spawn(char *argv[], char *buffer[], int i);


/* Dividing argv[1] into different operations as 2D array / double pointer */
char** argDivider(char* arg, int *counter);





#endif