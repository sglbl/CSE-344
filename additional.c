#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // Variadic function
#include <math.h>   // Math functions
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include <pthread.h> // Threads and mutexes
#include "additional.h" // Additional functions

static pthread_mutex_t mutex;
static int N, M;
// static char *filePath1, *filePath2, *outputPath;
static volatile sig_atomic_t didSigIntCome = 0;
// çç ascii check

int* openFiles(char *filePath1, char *filePath2, char *outputPath){
    int *fileDescs = calloc(3, sizeof(int));
    if ((fileDescs[0] = open(filePath1, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path1");
    if ((fileDescs[1] = open(filePath2, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path2");
    if ((fileDescs[2] = open(outputPath, O_RDWR | O_CREAT | 0666)) == -1) 
        errorAndExit("Error while opening file from output path");
    // filePath1 = filePath1, filePath2 = filePath2, outputPath = output;
    return fileDescs;
}

void readMatrices(int n, int m, int twoToN, int fileDescs[3], char matrixA[][twoToN], char matrixB[][twoToN]){
    N = n, M = m;
    int readedByte1, readedByte2;

    // Reading from file byte by byte
    for(int i = 0; i < twoToN; ++i){
        // printf("Supplier thread is reading...\n");
        if( (readedByte1 = read(fileDescs[0], matrixA[i], twoToN)) < 0 ){
            errorAndExit("Error while reading file1");
        }
        else if(readedByte1 == 0){
            break;
        }

        if( (readedByte2 = read(fileDescs[1], matrixB[i], twoToN)) < 0 ){
            errorAndExit("Error while reading file2");
        }
        else if(readedByte2 == 0){
            break;
        }
        // tprintf("Row[%d] from file1: %s, Row[%d] from file2: %s\n", i, buffer1[i], i, buffer2[i]);
    }
    tprintf("Files are readed\n");
    close(fileDescs[0]);    close(fileDescs[1]);    close(fileDescs[2]);   // closing file descriptors

}

void createThreads(int twoToN, char matrixA[twoToN][twoToN], char matrixB[twoToN][twoToN]){
    // Initializing mutex
    pthread_mutex_init(&mutex, NULL); 
    pthread_attr_t attr;
    pthread_t threads[M];

    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    printf("Matrix[0][3] is %c\n", matrixA[0][3]);

    Info info[M];
    /* create m threads */
    for (int i = 0; i < M; i++){  
        info[i].index = i;
        // info[i].
        // tprintf("Creating thread-%d, &info is %p\n", i, &info[i]);
        if( pthread_create(&threads[i], &attr, threadJob, (void*)&info[i]) != 0 ){ //if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
    }

    for(int i = 0; i < M; i++){
        if( pthread_join(threads[i], NULL) != 0 ){
            errorAndExit("pthread_join()");
        }
    }
    

}

void *threadJob(void *arg){
    Info *info = arg;
    int index = info->index;

    
    


    tprintf("Thread-%d has left\n", info->index);
    pthread_exit(NULL);
}

char *timeStamp(){
    time_t currentTime;
    currentTime = time(NULL);
    char *timeString = asctime( localtime(&currentTime) );

    // Removing new line character from the string
    int length = strlen(timeString);
    if (length > 0 && timeString[length-1] == '\n') 
        timeString[length - 1] = '\0';
    return timeString;
}

void tprintf(const char *restrict formattedStr, ...){
    // Variadic function to act like printf with timeStamp
    char newFormattedStr[150];
    snprintf(newFormattedStr, 150, "(%s) %s", timeStamp(), formattedStr);
    va_list argumentList;
    va_start(argumentList, newFormattedStr);
    vprintf( newFormattedStr, argumentList );
    va_end( argumentList );
}

void signalHandlerInitializer(){
    // Initializing signal action for SIGINT signal.
    struct sigaction actionForSigInt;
    memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    actionForSigInt.sa_handler = mySignalHandler; // Setting the handler function.
    actionForSigInt.sa_flags = 0; // No flag is set.
    if (sigaction(SIGINT, &actionForSigInt, NULL) < 0){ // Setting the signal.
        errorAndExit("Error while setting SIGINT signal. ");
    }
}

// Create a signal handler function
void mySignalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        didSigIntCome = 1;   // writing to static volative. If zero make it 1.
}

void errorAndExit(char *errorMessage){
    write(STDERR_FILENO, "Error ", 6);
    perror(errorMessage);
    // write(STDERR_FILENO, errorMessage, 6);
    exit(EXIT_FAILURE);
}