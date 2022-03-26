#ifndef SGPROCESSP_H_
#define SGPROCESSP_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#define TRUE 1
#define FALSE 0
#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3

//* Prints error and instruction about how to run app */
void printUsageAndExit();

void reader(int fileDescriptor, char *argv[]);

int spawn(char *argv[], char *buffer[], char* i);

void cleanTheOutputFile(char *argv[]);



#endif