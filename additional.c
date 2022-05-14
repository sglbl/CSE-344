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
static char **matrixC;
static volatile sig_atomic_t didSigIntCome = 0;
// çç ascii check

int* openFiles(char *filePath1, char *filePath2, char *outputPath){
    int *fileDescs = calloc(3, sizeof(int));
    if ((fileDescs[0] = open(filePath1, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path1");
    if ((fileDescs[1] = open(filePath2, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path2");
    if ((fileDescs[2] = open(outputPath, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) 
        errorAndExit("Error while opening file from output path");
    // filePath1 = filePath1, filePath2 = filePath2, outputPath = output;
    return fileDescs;
}

void readMatrices(int n, int m, int twoToN, int fileDescs[3], char matrixA[][twoToN], char matrixB[][twoToN]){
    N = n, M = m;
    int readByte1, readByte2;

    // Reading from file byte by byte
    for(int i = 0; i < twoToN; ++i){
        // printf("Supplier thread is reading...\n");
        if( (readByte1 = read(fileDescs[0], matrixA[i], twoToN)) < 0 ){
            errorAndExit("Error while reading file1");
        }
        else if(readByte1 == 0){
            break;
        }

        if( (readByte2 = read(fileDescs[1], matrixB[i], twoToN)) < 0 ){
            errorAndExit("Error while reading file2");
        }
        else if(readByte2 == 0){
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

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // Initializing matrix C
    matrixC = calloc(twoToN, sizeof(char*));
    for(int i = 0; i < twoToN; ++i){
        matrixC[i] = calloc(twoToN, sizeof(char));
    }

    /* create info ve allocate place to matrices for m threads */
    Info info[M];
    for (int i = 0; i < M; i++){  
        info[i].index = i;
        info[i].twoToN = twoToN;
        info[i].matrixA = calloc(twoToN, sizeof(char*));
        info[i].matrixB = calloc(twoToN, sizeof(char*));
        putMatrixToInfo(info[i], twoToN, matrixA, matrixB);
    }

    // Creating m threads
    for (int i = 0; i < M; i++){  
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

void putMatrixToInfo(Info info, int twoToN, char matrixA[twoToN][twoToN], char matrixB[twoToN][twoToN]){
    for(int j = 0; j < twoToN; j++){
        info.matrixA[j] = calloc(twoToN, sizeof(char));
        info.matrixB[j] = calloc(twoToN, sizeof(char));
        for(int k = 0; k < twoToN; k++){
            info.matrixA[j][k] = matrixA[j][k];
            info.matrixB[j][k] = matrixB[j][k];
            tprintf("Pointer is %c\n", info.matrixA[j][k]);
        }
    }

}

void *threadJob(void *arg){
    Info *info = arg;
    int columnIndex = info->index;
    
    /*  Every thread will calculate the one column of matrix C = matrixA * matrixB
            A           B           C
        [ a b c ]   [ k - - ]   [ x - -]
        [ d e f ] * [ l - - ] = [ y - -]
        [ g h j ]   [ m - - ]   [ z - -]
        For example 1st thread will calculate the 1st column of matrix C    
        x = a * k + b * l + c * m
        y = d * k + e * l + f * m
        z = g * k + h * l + j * m
    */

    // Thread_index calculates the indexTh column of matrixC
    for(int i = 0; i < info->twoToN; ++i){
        matrixC[i][columnIndex] = 0; /* row i, column index */
        for(int j = 0; j < info->twoToN; ++j){
            // Entering critical section.
            pthread_mutex_lock(&mutex);
            matrixC[i][columnIndex] += info->matrixA[i][j] * info->matrixB[j][columnIndex];
            pthread_mutex_unlock(&mutex);
        }
    }

    tprintf("Thread-%d calculated the column-%d of matrix C\n", columnIndex, columnIndex);
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