#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include "clientX.h"
#include "prerequisites.h"

static int serverPid = 0;
static volatile sig_atomic_t flag = 0; // Flag for signal handling
static char clientFifo[CLIENT_FIFO_PATH_NAME_SIZE];

static void removeClientFifo(){
    // timePrinter();
    // dprintf(STDOUT_FILENO, "Unlinking client fifo\n");
    unlink(clientFifo);
}

void handleDataFile(char *dataFilePath, char *serverFifoPath){
    int serverFileDesc;       // Directory stream file descriptor for file reading and writing
    int rowSize = 0;
    int *matrix = matrixReader(dataFilePath, &rowSize);

    signalHandlerInitializer();

    ClientRequest *request = 0;
    ServerResponse response;

    unsigned matrixSize = rowSize*rowSize;
    int requestSize = sizeof(*request) + matrixSize*sizeof(request->matrix[0]);
    request = malloc( requestSize );

    request->pid = getpid();
    request->sequenceLength = matrixSize;
    request->totalMatrixSize = matrixSize;
    request->rowSize = rowSize;
    for(int i = 0; i < matrixSize; i++)
        request->matrix[i] = matrix[i];
    free(matrix);

    strcpy( clientFifo, TEMP_CLIENT_FIFO );
    char *requestPid = itoaForAscii(request->pid);
    strcat( clientFifo, requestPid );
    free(requestPid);


    umask(0); // Set file creation mask to 0
    // Creating a fifo
    if( mkfifo(clientFifo, S_IRWXU) < 0 && errno != EEXIST ){
        perror("Error while creating server fifo. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    // In order to register remove fifo
    if(atexit(removeClientFifo) != 0){
        perror("Error while setting atexit function. ");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Client fifo: %s\n", clientFifo);
    // Opening server fifo in write mode
    if( (serverFileDesc = open(serverFifoPath, O_WRONLY, S_IWGRP)) == -1 ){
        perror("Error while opening server fifo to write.");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    // Printing the request with timestamp
    timePrinter();
    printf("Client PID#%d (%s) is submitting a %dx%d matrix\n", getpid(), dataFilePath, rowSize, rowSize);
    clock_t timeBegin = clock();		//Calculating read_array function's speed.

    if( write(serverFileDesc, request, requestSize) == -1 ){
        perror("Error while sending file size to server. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }
    free(request);
    if(flag == 1){
        // *didSigIntCome = 1;
        setbuf(stderr, NULL);
        timePrinter();
        dprintf(STDERR_FILENO, "SIGINT SIGNAL RECEIVED. SENDING TO SERVER\n");
        if(kill(serverPid , SIGINT) == -1) {
            timePrinter();
            perror("killing failed.\n");
            _exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }       

    // Opening this fifo to see the response.

    int clientFileDesc;
    if( (clientFileDesc = open(clientFifo, O_RDONLY)) == -1 ){
        perror("Error while opening client fifo to read. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    if( read(clientFileDesc, &response, sizeof(ServerResponse)) != sizeof(ServerResponse) ){
        if(flag == 1){
            // *didSigIntCome = 1;
            setbuf(stderr, NULL);
            timePrinter();
            dprintf(STDERR_FILENO, "SIGINT SIGNAL RECEIVED. SENDING TO SERVER\n");
            if(kill(serverPid , SIGINT) == -1) {
                timePrinter();
                perror("killing failed.\n");
                _exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }          
        perror("Error while reading from client fifo. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }
    serverPid = response.serverPid;
    printf("response server pid: %d\n", serverPid);
    if(flag == 1){
        // *didSigIntCome = 1;
        setbuf(stderr, NULL);
        timePrinter();
        dprintf(STDERR_FILENO, "SIGINT SIGNAL RECEIVED. SENDING TO SERVER\n");
        if(kill(serverPid , SIGINT) == -1) {
            timePrinter();
            perror("killing failed.\n");
            _exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }   

    clock_t time = clock() - timeBegin;
	double seconds = ((double)time)/CLOCKS_PER_SEC;
    // Printing the request with timestamp
    
    if(flag == 1){
        if(kill(serverPid , SIGINT) == -1) {
            timePrinter();
            perror("killing failed.\n");
            _exit(EXIT_FAILURE);
        }
    }
    timePrinter();
    if(response.isInvertible == TRUE)
        printf("Client PID#%d: the matrix is invertible, total time %f seconds, goodbye.\n", getpid(), seconds );
    else
        printf("Client PID#%d: the matrix is not invertible, total time %f seconds, goodbye.\n", getpid(), seconds );   

    // close(serverFileDesc);
    exit(EXIT_SUCCESS);
}

void signalHandlerInitializer(){
    // Initializing siggnal action for SIGINT signal.
    struct sigaction actionForSigInt;
    memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    actionForSigInt.sa_handler = sg_signalHandler; // Setting the handler function.
    actionForSigInt.sa_flags = 0; // No flag is set.
    if (sigaction(SIGINT, &actionForSigInt, NULL) < 0){ // Setting the signal.
        perror("Error while setting SIGINT signal. ");
        exit(EXIT_FAILURE);
    }
}

// Create a signal handler function
void sg_signalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        flag = 1;   // writing to static volative. If zero make it 1.
}

void timePrinter(){
    time_t currentTime;
    currentTime=time(NULL);
    char *timeString = asctime( localtime(&currentTime) );

    // Removing new line character from the string
    int length = strlen(timeString);
    if (length > 0 && timeString[length-1] == '\n') 
        timeString[length - 1] = '\0';
    dprintf(STDOUT_FILENO, "(%s) ", timeString);
}

int * matrixReader(char* dataFilePath, int *rowSize){
    struct stat statOfFile;                 // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int dataFileDesc;
    if( stat(dataFilePath, &statOfFile) < 0){
        perror("Error while opening file to read. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    // Opening file in read mode
    if( (dataFileDesc = open(dataFilePath, O_RDONLY, S_IWGRP)) == -1 ){
        perror("Error while opening file to read. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    int readedBytes;
    int totalSize = statOfFile.st_size;
    if(totalSize < 8){
        write(STDERR_FILENO, "Size of matrix has to be at least 2*2\n", 39);
        exit(EXIT_FAILURE);
    }
    
    char *buffer = (char *)calloc(statOfFile.st_size, sizeof(char));
    while( (readedBytes = read(dataFileDesc, buffer, statOfFile.st_size)) == -1 && errno == EINTR ){}
    if(readedBytes <= 0){
        perror("Error! File is empty.\n");
        exit(EXIT_FAILURE);
    }

    char *tempBuffer2 = buffer; // At the end, to free
    // Finding size of matrix and rowSize
    totalSize = 0;
    char *tempBuffer = calloc(statOfFile.st_size, sizeof(char));
    strcpy(tempBuffer, buffer);
    char *token = strtok(tempBuffer, ",;\n\r\0");
    for(int i=0; token != NULL; i++){     //Name of the children will be R_i
        totalSize++;
        token = strtok(NULL, ",;\n\r\0");
    }
    free(tempBuffer);
    // Checking if the matrix is square
    int squareMatrix = FALSE;
    for(int n = 0; n < totalSize; n++){
        if( n*n == totalSize){
            *rowSize = n;
            squareMatrix = TRUE;
            break;
        }
    }
    // Now total size is the number of integers in the matrix
    if(squareMatrix == FALSE){
        write(STDERR_FILENO, "The matrix in the file is not nXn square Matrix\n", 49);
        exit(EXIT_FAILURE);
    }

    int *matrix = (int*)calloc( (*rowSize)*(*rowSize), sizeof(int));
    token = strtok(buffer, ",;\n\r\0");
    for(int i=0; i < (*rowSize)*(*rowSize) && token != NULL; i++){     //Name of the children will be R_i
        // Allocating memory for buffer which will store the content of input file
        matrix[i] = atoi(token);
        token = strtok(NULL, ",;\n\r\0");
    }
    // Reading is completed. Closing the file.
    if( close(dataFileDesc) == -1 ){   
        perror("Error while closing the file. ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }
    free(tempBuffer2);
    return matrix;
}

char* itoaForAscii(int number){
    if(number == 0){
        char* string = calloc(2, sizeof(char));
        string[0] = '0';    string[1] = '\0';
        return string;
    }
    int digitCounter = 0;
    int temp = number;
    while(temp != 0){
        temp /= 10;
        digitCounter++;
    }
    
    char* string = calloc(digitCounter+1, sizeof(char) );
    for(int i = 0; i < digitCounter; i++){
        char temp = (number % 10) + '0';
        string[digitCounter-i-1] = temp;
        number /= 10;
    }
    string[digitCounter] = '\0';
    return string;
}
