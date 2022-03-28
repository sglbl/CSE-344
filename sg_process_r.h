#ifndef SGPROCESSR_H_
#define SGPROCESSR_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#define TRUE 1
#define FALSE 0
#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3

/* Prints Children information
   @example Created R_1 with (123,115,103), (108,98,108), ..., (100,117,46) */
void printChildInfo(int childNumber);

/* Converts integer to string(char*) for non-negative ascii values. */
char* itoaForAscii(int number);

double** findCovarianceMatrix(int i);

void writeToFile(int fileDesc, double **covarianceMatrix);

#endif