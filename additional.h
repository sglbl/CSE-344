#ifndef ADDITIONAL_H_
#define ADDITIONAL_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

void createSemSet();

void createThreads(int C, int N, char *path);

void errorAndExit(char *errorMessage);


#endif