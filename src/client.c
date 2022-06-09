#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // Variadic function
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include <pthread.h> // Threads and mutexes
#include <arpa/inet.h> // Parser for ip adress
#include "../include/client.h"
#include "../include/common.h"

static volatile sig_atomic_t didSigIntCome = 0;

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    int option, portNo = -1;
    char *filePath, *ipv4;

    while ((option = getopt(argc, argv, "r:q:s:")) != -1){
        switch (option){
            case 'r':
                filePath = optarg;
                break;
            case 'q':
                portNo = atoi(optarg);
                break;
            case 's':
                ipv4 = optarg;
                break;
            default:
                write(STDERR_FILENO, "Error with arguments\nUsage: ./client -r requestFile -q PORT -s IP\n", 66);
                exit(EXIT_FAILURE);
        }
    }

    if( portNo <= 2000){
        write(STDERR_FILENO, "Error. It should be p > 2000, t >= 5\nUsage: ./client -r requestFile -q PORT -s IP\n", 82);
        exit(EXIT_FAILURE);
    }

    signalHandlerInitializer();
    int requestFd = openRequestFile(filePath);
    // Number of threads will be equal to the number of requests
    int numberOfThreads = getNumberOfRequests(filePath, requestFd);
    createThreads(portNo, ipv4, requestFd, numberOfThreads);



    return 0;
}

int openRequestFile(char *filePath){
    int requestFileFd;
    if((requestFileFd = open(filePath, O_RDONLY, 0666)) < 0)
        errorAndExit("Error while opening request file");
    return requestFileFd;
}

int getNumberOfRequests(char *filePath, int requestFd){
    // Reading number of lines in the request file by using stat
    struct stat statOfFile;
    stat(filePath, &statOfFile);
    printf("Size is %ld\n", statOfFile.st_size);

    lseek(requestFd, 0, SEEK_SET);  
    char *buffer = calloc( statOfFile.st_size , sizeof(char));   // Buffer that will be used to temporarily storing.
    // Count number of new lines in file
    int numberOfLines = 1; // first line
    for(int i = 0; i < statOfFile.st_size; i++){
        if(read(requestFd, &buffer[i], 1) < 0)
            errorAndExit("Error while reading request file");
        if(buffer[i] == '\n')
            numberOfLines++;
    }

    // setting cursor to the beginning of file with lseek / rewinding
    lseek(requestFd, 0, SEEK_SET);  
    printf("Number of lines is %d\n", numberOfLines);
    return numberOfLines;
}

void createThreads(int portNo, char *ipv4, int fileDesc, int numOfThreads){
    // Initializing mutex and conditional variable
    // pthread_mutex_init(&csMutex, NULL); 
    // pthread_mutex_init(&barrierMutex, NULL); 
    // pthread_cond_init(&barrierCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[numOfThreads];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    typedef int Info;
    Info info[numOfThreads];
    memset(info, 0, sizeof(info));

    // Creating threads
    for (int i = 0; i < numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, doClientJob, (void*)&info[i]) != 0 ){ // if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
    }

    for(int i = 0; i < numOfThreads; i++){
        if( pthread_join(threads[i], NULL) != 0 ){
            errorAndExit("pthread_join()");
        }
    }

    if(didSigIntCome == 1 ){
        write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
        exit(EXIT_SUCCESS);
    }

    // Freeing allocated memory of matrix C and output matrix
    exit(EXIT_SUCCESS);
}

void *doClientJob(void *arg){
    // extract struct as char *filePath, int portNo, char *ipv4



    pthread_exit(NULL);
}

static void exitingJob(){
    //çç free and close
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
