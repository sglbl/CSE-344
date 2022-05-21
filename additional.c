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
static ComplexNumber **outputMatrix;
static Info *info;
static int part1FinishedThreads;
static int N, M;
static int **matrixC;
static int outputFileDesc;
static volatile sig_atomic_t didSigIntCome = 0;

static void exitingJob(){
    // Freeing allocated memory of info
    for (int i = 0; i < M; i++){  
        for(int j = 0; j < pow(2,N); ++j){
            free(info[i].matrixA[j]);
            free(info[i].matrixB[j]);
        }
        free(info[i].matrixA);
        free(info[i].matrixB);
    }
    free(info);

    // Freeing allocated memory
    freeAllocatedMemory(pow(2,N)/* Size of matrix row*/);
    // Closing output file
    close(outputFileDesc);
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
    // Also initalizing atexit function.
    if(atexit(exitingJob) != 0){
        errorAndExit("Error while setting atexit function. ");
    }
}

// Create a signal handler function
void mySignalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        didSigIntCome = 1;   // writing to static volative. If zero make it 1.
}

void openFiles(char *filePath1, char *filePath2, char *outputPath, int fileDescs[3]){
    if ((fileDescs[0] = open(filePath1, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path1");
    if ((fileDescs[1] = open(filePath2, O_RDONLY, S_IRUSR | S_IRGRP | S_IRGRP)) == -1) 
        errorAndExit("Error while opening file from path2");
    if ((fileDescs[2] = open(outputPath, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) 
        errorAndExit("Error while opening file from output path");
}

void readMatrices(int n, int m, int twoToN, int fileDescs[3], int matrixA[][twoToN], int matrixB[][twoToN]){
    N = n, M = m;
    int readByte1, readByte2;
    char buffer1[twoToN][twoToN], buffer2[twoToN][twoToN];

    // Reading from file byte by byte
    for(int i = 0; i < twoToN; ++i){
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

        // tprintf("Row[%d] from file1: %d, Row[%d] from file2: %d\n", i, matrixA[i][0], i, matrixB[i][0]);
    }
    tprintf("Two matrices of size %dx%d have been read. The number of threads is %d\n", twoToN, twoToN, M);
    close(fileDescs[0]);    close(fileDescs[1]); // closing file descriptors of readed files
    outputFileDesc = fileDescs[2];    
}

void createThreads(int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]){
    double startTime = clock();
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
    outputMatrix = calloc(twoToN, sizeof(ComplexNumber*));
    for(int i = 0; i < twoToN; ++i){
        matrixC[i] = calloc(twoToN, sizeof(int));
        outputMatrix[i] = calloc(twoToN, sizeof(ComplexNumber));
    }

    /* create info ve allocate place to matrices for m threads */
    info = calloc(M, sizeof(Info));
    for (int i = 0; i < M; i++){  
        info[i].threadId = i;
        info[i].twoToN = twoToN;
        info[i].numOfColumnToCalculate = twoToN / M;
        if( i == M-1 && twoToN % M != 0 ){
            // There will be remaining columns so let last thread handle them.
            info[i].numOfColumnToCalculate = twoToN - (int)((M-1)*floor(twoToN/M));
        }
        info[i].matrixA = calloc(twoToN, sizeof(int*));
        info[i].matrixB = calloc(twoToN, sizeof(int*));
        putMatrixToInfo(info[i], twoToN, matrixA, matrixB);
    }

    // Creating m threads
    for (int i = 0; i < M; i++){  
        if( pthread_create(&threads[i], &attr, threadJob, (void*)&info[i]) != 0 ){ // if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
    }

    for(int i = 0; i < M; i++){
        if( pthread_join(threads[i], NULL) != 0 ){
            errorAndExit("pthread_join()");
        }
    }

    if(didSigIntCome == 1 ){
        write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
        exit(EXIT_SUCCESS);
    }

    writeToCsv(twoToN /* row and column size of square matrix */);

    tprintf("The process has written the output file. The total time spent is %.3f seconds.\n", (double)(clock() - startTime) / CLOCKS_PER_SEC);

    // Printing matrix C to stdout
    // matrixPrinter(twoToN, matrixC);

    // Freeing allocated memory of matrix C and output matrix
    exit(EXIT_SUCCESS);
}

void putMatrixToInfo(Info info, int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]){
    // Storing matrix inside struct
    for(int j = 0; j < twoToN; j++){
        info.matrixA[j] = calloc(twoToN, sizeof(int));
        info.matrixB[j] = calloc(twoToN, sizeof(int));
        for(int k = 0; k < twoToN; k++){
            info.matrixA[j][k] = matrixA[j][k];
            info.matrixB[j][k] = matrixB[j][k];
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
    Info *infoArg = arg;
    setbuf(stdout, NULL);

    /*  Every thread will calculate the some column of matrix C = matrixA * matrixB
        For example if 2^n was 3 (it cannot be, just an example), matrix row and column would become 3
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

    for(int i = 0; i < infoArg->numOfColumnToCalculate; ++i){
        // Thread_(info->index) will calculate (info->numOfColumnToCalculate) columns
        int columnIndex = infoArg->threadId * infoArg->numOfColumnToCalculate + i;
        if(infoArg->threadId == M-1){  // For the remaining columns
            columnIndex = infoArg->threadId * (infoArg-1)->numOfColumnToCalculate + i; 
        }
        for(int j = 0; j < infoArg->twoToN; ++j){ 
            matrixC[j][columnIndex] = 0; /* row j, column columnIndex */
            for(int k = 0; k < infoArg->twoToN; ++k){
                // Entering critical section.
                pthread_mutex_lock(&csMutex);
                matrixC[j][columnIndex] += infoArg->matrixA[j][k] * infoArg->matrixB[k][columnIndex];
                if( didSigIntCome == 1 ){
                    write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
                    exit(EXIT_SUCCESS);
                }
                pthread_mutex_unlock(&csMutex);
            }
        }
    }

    double seconds = (double)(clock() - timeBegin)/CLOCKS_PER_SEC;
    tprintf("Thread %d has reached the rendezvous point in %.5f seconds\n", infoArg->threadId, seconds);

    // Barrier    
    barrier();

    timeBegin = clock();

    // Part2
    tprintf("Thread %d is advancing to the second part\n", infoArg->threadId);
    for(int i = 0; i < infoArg->numOfColumnToCalculate; ++i){
        int columnIndex = infoArg->threadId * infoArg->numOfColumnToCalculate + i; 
        if(infoArg->threadId == M-1){  // For the remaining columns
            columnIndex = infoArg->threadId * (infoArg-1)->numOfColumnToCalculate + i; 
        }
        for(int newRow = 0; newRow < infoArg->twoToN; ++newRow){
            // for(int newColumn; newColumn < info->twoToN; ++newColumn)
            // NO NEED TO ITERATE ON COLUMNS LIKE ABOVE. THIS THREAD ONLY CALCULATES "COLUMNINDEX" COLUMNS
            outputMatrix[newRow][columnIndex].real = 0;
            outputMatrix[newRow][columnIndex].imag = 0;
            for(int row = 0; row < infoArg->twoToN; ++row){
                for(int col = 0; col < infoArg->twoToN; ++col){
                    // double complex number = I * (-2 * M_PI * ((newRow * row + newCol * col) / (twoToN*1.0)));
                    // double complex matrixIndexValue = matrixC[row][col] + 0 * I;
                    // dftIndexValue += ( matrixIndexValue * cexp(number));
                    pthread_mutex_lock(&csMutex); // Don't let other threads to free too.
                    double radian = ( 2 * M_PI * ( (newRow * row + columnIndex * col)/(infoArg->twoToN*1.0) ) );
                    outputMatrix[newRow][columnIndex].real += ( matrixC[row][col] * cos(radian) );
                    outputMatrix[newRow][columnIndex].imag += ( matrixC[row][col] * sin(radian) );
                    if( didSigIntCome == 1 ){
                        write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
                        exit(EXIT_SUCCESS);
                    }
                    pthread_mutex_unlock(&csMutex); // Don't let other threads to free too.
                }
            }
        }
    
    }

    seconds = (double)(clock() - timeBegin)/CLOCKS_PER_SEC;
    tprintf("Thread %d has finished the second part in %.3f seconds. \n", infoArg->threadId, seconds);
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


void freeAllocatedMemory(int twoToN){
    // Freeing allocated memory of matrixC and outputMatrix
    for(int i = 0; i < M; i++){}

    didSigIntCome = 0;
    for(int i = 0; i < twoToN; ++i){
        free(matrixC[i]);
        // free(outputMatrix[i]);
    }
    free(matrixC);
    // free(outputMatrix);
}

void writeToCsv(int size){
    // Writing twoToN * twoToN outputMatrix to csv file
    char buffer[100];
    for(int i = 0; i < size; ++i){
        for(int j = 0; j < size; ++j){
            if(j != size-1) 
                sprintf(buffer, "%.3f + %.3fi,", outputMatrix[i][j].real, outputMatrix[i][j].imag);
            else
                sprintf(buffer, "%.3f + %.3fi",  outputMatrix[i][j].real, outputMatrix[i][j].imag);
            if( write(outputFileDesc, buffer, strlen(buffer)) < 0 )
                errorAndExit("Error while writing to output file");
        }
        if( write(outputFileDesc, "\n", 1) < 0 )
            errorAndExit("Error while writing to output file");
    }

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