#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/file.h> // For flock (to apply/remove an consultative lock on an open file)
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include <pthread.h> // Threads
#include <sys/types.h> // For semaphore
#include <sys/ipc.h>  // For semaphore
#include <sys/sem.h>  // For semaphore
#include "additional.h"

SemUnion semUnionArgument;
int semid;
static int C, N;
static char *filePath;

void createSemSet(){
    // setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    // Semaphores are initialized by creating a semaphore set with 2 semaphore
    if ((semid = semget(IPC_PRIVATE/* Private key */,  2 /* Number of sem to cretae */, 0666 | IPC_CREAT)) == -1) {
        errorAndExit("semget");
    }

    /* initialize semaphore index0 to 0 (semUNionArgument.val) */
    semUnionArgument.val = 0;
    if (semctl(semid, 0 /* semaphore with index 0 in set */ , SETVAL /* operation to be performed */, semUnionArgument /* setting this argument with setval */) == -1) {
        errorAndExit("semctl error");
    }

    /* initialize semaphore index1 to 0 (semUNionArgument.val) */
    if (semctl(semid, 1 /* semaphore with index 0 in set */ , SETVAL /* operation to be performed */, semUnionArgument /* setting this argument with setval */) == -1) {
        errorAndExit("semctl error");
    }

}

void createThreads(int nNumber, int cNumber, char *path){
    pthread_attr_t attr;
    pthread_t consumers[nNumber];
    pthread_t theProducer;

    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    N = nNumber; C = cNumber; filePath = path;
    if( pthread_create(&theProducer, NULL, supplierThread, (void*)0) != 0 ){ //if returns 0 it's okay.
            errorAndExit("pthread_create()");
    }

    int *returnStatus2[C];
    /* create the workers, then exit */
    for (int i = 0; i < C; i++){  
        // int info;
        // if( pthread_create(&consumers[i], &attr, consumerThread, (void*)&info) != 0 ){ //if returns 0 it's okay.
        if( pthread_create(&consumers[i], &attr, consumerThread, (void*)(long)i) != 0 ){ //if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
        
    }

    if( pthread_detach(theProducer) != 0 ){
        errorAndExit("pthread_detach()");
    }


    for(int i = 0; i < C; i++)
        if( pthread_join(consumers[i], (void**) &returnStatus2[i]) != 0 ){
            errorAndExit("pthread_join()");
        }

}

void *supplierThread(void *arg){
    // Opening file using path
    int readedByte, fileDesc;
    if ((fileDesc = open(filePath, O_RDONLY, S_IRUSR)) == -1) {
        errorAndExit("Error while opening file");
    }

    // Creating buffer
    char *buffer = calloc(C, sizeof(char));

    // Reading from file byte by byte
    for(int i = 0; ; ++i){
        printf("Supplier thread is reading...\n");
        if( (readedByte = read(fileDesc, &buffer[i], 1)) < 0 ){
            errorAndExit("Error while reading file");
        }
        else if(readedByte == 0){
            tprintf("Supplier: End of file\n");
            tprintf("i = %d\n", i);
            break;
        }

        // Getting the value of semaphores with semctl
        int valOf1 = 0, valOf2 = 0;
        if( (valOf1 = semctl(semid, 0, GETVAL)) == -1 ) errorAndExit("semctl error for sem1");
        if( (valOf2 = semctl(semid, 1, GETVAL)) == -1 ) errorAndExit("semctl error for sem2");

        tprintf("Supplier: read from input a '%c'. Current amounts: %d x '1', %d x '2'.\n", buffer[i], valOf1, valOf2);
        if(buffer[i] == '1')        postSemaphore(/* sem number */0);
        else if(buffer[i] == '2')   postSemaphore(/* sem number */1);
        // else /* Nothing readed */   break;       

        valOf1 = 0, valOf2 = 0;
        if( (valOf1 = semctl(semid, 0, GETVAL)) == -1 ) errorAndExit("semctl error for sem1");
        if( (valOf2 = semctl(semid, 1, GETVAL)) == -1 ) errorAndExit("semctl error for sem2");
        
        tprintf("Supplier: delivered a '%c'. Post-delivery amounts: %d x '1', %d x '2'.\n", buffer[i], valOf1, valOf2);
    }
    close(fileDesc);
    free(buffer);
    pthread_exit(NULL);
    // Removing semaphore set
    // if (semctl(semid, 0, IPC_RMID, arg) == -1)  errorAndExit("Semctl error for sem1");
    // if (semctl(semid, 1, IPC_RMID, arg) == -1)  errorAndExit("Semctl error for sem2");
}

void *consumerThread(void *arg){
    long i = (long)arg; // On most systems, sizeof(long) == sizeof(void *)
    for(int j = 0; j < N; j++){ // Each consumer thread will loop N times.

        int valOf1 = 0, valOf2 = 0;
        if( (valOf1 = semctl(semid, 0, GETVAL)) == -1 ) errorAndExit("semctl error for sem1");
        if( (valOf2 = semctl(semid, 1, GETVAL)) == -1 ) errorAndExit("semctl error for sem2");
        tprintf("Consumer-%ld at iteration %d (waiting). "
            "Current amounts: %d x '1', %d x '2'.\n", i, j, valOf1, valOf2);

        waitSemaphoreForBoth();

        tprintf("Consumer-%ld at iteration %d (consumed). "
            "Post-consumption amounts: %d x '1', %d x '2'.\n", i, j, valOf1, valOf2);


        valOf1 = 0, valOf2 = 0;
        if( (valOf1 = semctl(semid, 0, GETVAL)) == -1 ) errorAndExit("semctl error for sem1");
        if( (valOf2 = semctl(semid, 1, GETVAL)) == -1 ) errorAndExit("semctl error for sem2");
        
    }
    tprintf("Consumer-%ld has left\n", i);
    pthread_exit(NULL);
}

void postSemaphore(int semIndex){
    // INCREMENTING THE VALUE OF SEMAPHORE WITH SEMCTL
    struct sembuf semOper;
    semOper.sem_num = semIndex; /* sem operation for index0 */ semOper.sem_op = 1; /* Post sem */ semOper.sem_flg = 0;
    if( semop(semid, &semOper, 1/* Num of operation*/) == -1 ){
        errorAndExit("semop");
    }
}

void waitSemaphoreForBoth(){
    // DECREMENTING THE VALUE OF SEMAPHORE WITH SEMCTL
    struct sembuf semOper[2]; // Atomically performing for both of them to decrease by 1
    semOper[0].sem_num = 0; /* sem operation for semaphore in index0 */ semOper[0].sem_op = -1; /* Wait sem */ semOper[0].sem_flg = 0;
    // semOper[1].sem_num = 0; /* sem operation for index1 */ semOper[1].sem_op = -1; /* make sem++ */ semOper[1].sem_flg = 0;

    // semOper[0].sem_num = 1; /* sem operation for index0 */ semOper[0].sem_op = 1; /* Wait sem */ semOper[0].sem_flg = 0;
    semOper[1].sem_num = 1; /* sem operation for semaphore in index1 */ semOper[1].sem_op = -1; /* make sem++ */ semOper[1].sem_flg = 0;

    if( semop(semid, semOper /* operations to be performed */, 2/* Num of operations*/) == -1 ){
        errorAndExit("semop");
    }
}

char *timeStamp(){
    time_t currentTime;
    currentTime=time(NULL);
    char *timeString = asctime( localtime(&currentTime) );

    // Removing new line character from the string
    int length = strlen(timeString);
    if (length > 0 && timeString[length-1] == '\n') 
        timeString[length - 1] = '\0';
    return timeString;
}

void tprintf(const char *restrict formattedStr, ...){
	va_list ap;
	va_start(ap, formattedStr);
    // Adding time stamp to the string
    char *newFormattedStr = NULL;
    asprintf(&newFormattedStr, "(%s) %s", timeStamp(), formattedStr);
    // Printing the string
	vfprintf(stdout, newFormattedStr, ap);
    free(newFormattedStr);
	va_end(ap);
}

void errorAndExit(char *errorMessage){
    write(STDERR_FILENO, "Error ", 6);
    perror(errorMessage);
    exit(EXIT_FAILURE);
}