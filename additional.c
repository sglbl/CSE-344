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

static pthread_mutex_t csMutex;
static pthread_mutex_t barrierMutex;
static pthread_cond_t barrierCond;
static int part1FinishedThreads;
static int N, M;
static int **matrixC;
static volatile sig_atomic_t didSigIntCome = 0;

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

int* openFiles(char *filePath1, char *filePath2, char *outputPath){
    int *fileDescs = calloc(3, sizeof(int));
    if ((fileDescs[0] = open(filePath1, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path1");
    if ((fileDescs[1] = open(filePath2, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path2");
    if ((fileDescs[2] = open(outputPath, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) 
        errorAndExit("Error while opening file from output path");
    return fileDescs;
}

void readMatrices(int n, int m, int twoToN, int fileDescs[3], int matrixA[][twoToN], int matrixB[][twoToN]){
    N = n, M = m;
    int readByte1, readByte2;
    char buffer1[twoToN][twoToN], buffer2[twoToN][twoToN];

    // Reading from file byte by byte
    for(int i = 0; i < twoToN; ++i){
        // printf("Supplier thread is reading...\n");
        if( (readByte1 = read(fileDescs[0], buffer1[i], twoToN)) < 0 )
            errorAndExit("Error while reading file1");
        else if(readByte1 == 0)
            break;

        if( (readByte2 = read(fileDescs[1], buffer2[i], twoToN)) < 0 )
            errorAndExit("Error while reading file2");
        else if(readByte2 == 0)
            break;
        
        for(int j = 0; j < twoToN; ++j){
            if(buffer1[i][j] < 0 || buffer1[i][j] > 256 || buffer2[i][j] < 0 || buffer2[i][j] > 256){
                errorAndExit("File contains non-ascii value. Put a valid file ");
            }
            // Char to integer conversion [No need explicit casting]
            matrixA[i][j] = buffer1[i][j]; 
            matrixB[i][j] = buffer2[i][j];
        }

        tprintf("Row[%d] from file1: %d, Row[%d] from file2: %d\n", i, matrixA[i][0], i, matrixB[i][0]);
    }
    tprintf("Two matrices of size %dx%d have been read. The number of threads is %d\n", twoToN, twoToN, M);
    close(fileDescs[0]);    close(fileDescs[1]);    close(fileDescs[2]);   // closing file descriptors

}

void createThreads(int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]){
    // Initializing mutex and conditional variable
    pthread_mutex_init(&csMutex, NULL); 
    pthread_mutex_init(&barrierMutex, NULL); 
    pthread_cond_init(&barrierCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[M];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // Initializing matrix C
    matrixC = calloc(twoToN, sizeof(int*));
    for(int i = 0; i < twoToN; ++i){
        matrixC[i] = calloc(twoToN, sizeof(int));
    }

    /* create info ve allocate place to matrices for m threads */
    Info info[M];
    for (int i = 0; i < M; i++){  
        info[i].index = i;
        info[i].twoToN = twoToN;
        info[i].numOfColumnToCalculate = twoToN / M;
        info[i].matrixA = calloc(twoToN, sizeof(int*));
        info[i].matrixB = calloc(twoToN, sizeof(int*));
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

    // Printing matrix C to stdout
    matrixPrinter(twoToN, matrixC);
    
}

void putMatrixToInfo(Info info, int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]){
    for(int j = 0; j < twoToN; j++){ //çç free
        info.matrixA[j] = calloc(twoToN, sizeof(int));
        info.matrixB[j] = calloc(twoToN, sizeof(int));
        for(int k = 0; k < twoToN; k++){
            info.matrixA[j][k] = matrixA[j][k];
            info.matrixB[j][k] = matrixB[j][k];
            // tprintf("Pointer is %c\n", info.matrixA[j][k]);
        }
    }

}

void matrixPrinter(int twoToN, int **matrix){
    for(int i = 0; i < twoToN; ++i){
        for(int j = 0; j < twoToN; ++j){
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}

void *threadJob(void *arg){
    Info *info = arg;
    
    /*  Every thread will calculate the one column of matrix C = matrixA * matrixB
            A           B           C
        [ a b c ]   [ k - - ]   [ x - -]
        [ d e f ] * [ l - - ] = [ y - -]
        [ g h j ]   [ m - - ]   [ z - -]
        For example 1st thread will calculate the 1st column (index=0) of matrix C    
        x = a * k + b * l + c * m
        y = d * k + e * l + f * m
        z = g * k + h * l + j * m
    */

    // Get time difference to find the time taken by each thread
    clock_t timeBegin = clock();

    for(int i = 0; i < info->numOfColumnToCalculate; ++i){
        // Thread_index calculates the indexTh column of matrixC
        for(int j = 0; j < info->twoToN; ++j){
            int columnIndex = info->index * info->numOfColumnToCalculate + i;
            matrixC[j][columnIndex] = 0; /* row i, column index */
            for(int k = 0; k < info->twoToN; ++k){
                // Entering critical section.
                pthread_mutex_lock(&csMutex);
                matrixC[j][columnIndex] += info->matrixA[j][k] * info->matrixB[k][columnIndex];
                pthread_mutex_unlock(&csMutex);
            }
        }
    }

    tprintf("Thread %d calculated %d columns of matrix C\n", info->index, info->numOfColumnToCalculate);

    // Barrier    
    barrier();
	double seconds = (double)(clock() - timeBegin)/CLOCKS_PER_SEC;
    tprintf("Thread %d has reached the rendezvous point in %.5f seconds\n", info->index, seconds);

    // Part2
    tprintf("Thread %d is advancing to the second part\n", info->index);





    pthread_exit(NULL);
}

void barrier(){
    pthread_mutex_lock(&barrierMutex);
    ++part1FinishedThreads;
    while(TRUE){ // Not busy waiting. Using conditional variable + mutex for monitoring
        if(part1FinishedThreads == M /* Number of threads */){
            part1FinishedThreads = 0;
            pthread_cond_broadcast(&barrierCond); // signal to all threads to wake them up
            break;
        }
        else{
            pthread_cond_wait(&barrierCond, &barrierMutex);
            break;
        }
    }
    pthread_mutex_unlock(&barrierMutex);
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

void errorAndExit(char *errorMessage){
    write(STDERR_FILENO, "Error ", 6);
    perror(errorMessage);
    // write(STDERR_FILENO, errorMessage, 6);
    exit(EXIT_FAILURE);
}