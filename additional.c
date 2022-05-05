#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int C, N;
static char *filePath;

// void createSemSet(){
//     key_t key;
//     int semid;
//     union semun arg;

//     /* create a semaphore set with 1 semaphore: */
//     if ((semid = semget(IPC_PRIVATE/* Private key */,  2 /* # of sem */, 0666 | IPC_CREAT)) == -1) {
//         errorAndExit("semget");
//         exit(1);
//     }

//     /* initialize semaphore #0 to 1: */
//     arg.val = 1;
//     if (semctl(semid, 0 /* # of particular sem in set */ , SETVAL /* operation to be performed */, arg /* setting this argument with setval */) == -1) {
//         perror("semctl");
//         exit(1);
//     }

// }

int numArrived = 0;       /* number who have arrived */

void *supplierThread(void *arg){
    // Opening file using path
    int readedByte, fileDesc;
    if ((fileDesc = open(filePath, O_RDONLY, S_IRUSR)) == -1) {
        printf("Fİle is %s\n", filePath);
        errorAndExit("Error while opening file");
    }

    // Creating buffer
    char *buffer = malloc(sizeof(char) * C);

    // Reading from file byte by byte
    for(int i = 0; ; ++i){

        if( (readedByte = read(fileDesc, &buffer[i], 1)) < 0 ){
            errorAndExit("Error while reading file");
        }
        else if(readedByte == 0){
            dprintf(STDOUT_FILENO, "Supplier: End of file\n");
            printf("i = %d\n", i);
            break;
        }
        dprintf(STDOUT_FILENO, "The byte readed is %c\n", buffer[i]);
    }

}

void *consumerThread(void *arg){
    for(int j = 0; j < N; j++){ // Each consumer thread will loop N times.
        int *i = arg;
        printf("cosnumed\n");
        // dprintf(STDOUT_FILENO, "Consumer-%d at iteration %d (waiting). "
        // "Current amounts: %d x '1', %d x '2'.\n", *i, j, amountOf1, amountOf2);

        // dprintf(STDOUT_FILENO, "Consumer-%d at iteration %d (consumed). "
        // "Post-consumption amounts: %d x '1', %d x '2'.\n", *i, j, amountOf1, amountOf2);
    }


}

void createThreads(int nNumber, int cNumber, char *path){
    int *returnStatus;
    pthread_attr_t attr;
    pthread_t consumers[nNumber];
    pthread_t master;

    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    N = nNumber; C = cNumber; filePath = path;
    if( pthread_create(&master, NULL, supplierThread, (void*)0) != 0 ){ //if returns 0 it's okay.
            dprintf(STDOUT_FILENO, "ERROR from pthread_create()");
            exit(EXIT_FAILURE);
    }
    if( pthread_join(master, (void**) &returnStatus) != 0 ){
        dprintf(STDOUT_FILENO, "ERROR from pthread_join()");
        exit(EXIT_FAILURE);
    }
    
    // printf("Return status is %d\n", *returnStatus);

    /* create the workers, then exit */
    for (int i = 0; i < C; i++){  
        int *returnStatus2;
        // int info;
        // if( pthread_create(&consumers[i], &attr, consumerThread, (void*)&info) != 0 ){ //if returns 0 it's okay.
        if( pthread_create(&consumers[i], &attr, consumerThread, (void*)i) != 0 ){ //if returns 0 it's okay.
            dprintf(STDOUT_FILENO,"ERROR from pthread_create()");
            exit(EXIT_FAILURE);
        }
        
        if( pthread_join(consumers[i], (void**) &returnStatus2) != 0 ){
            dprintf(STDOUT_FILENO,"ERROR from pthread_join()");
            exit(EXIT_FAILURE);
        }
        
        // int intMult = *mult;
        // free(returnStatus);
    }



}


void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}