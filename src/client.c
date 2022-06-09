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
static pthread_mutex_t csMutex;
static pthread_mutex_t barrierMutex;
static pthread_cond_t barrierCond;
static int s_numOfThreads;

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

    // Reading number of lines in the request file by using stat
    struct stat statOfFile;
    stat(filePath, &statOfFile);
    // Buffer will be used to temporarily storing.
    char *buffer = calloc( statOfFile.st_size , sizeof(char));

    // Number of threads will be equal to the number of requests
    int numberOfThreads = getNumberOfRequests(statOfFile.st_size, requestFd, buffer);
    String lineData[s_numOfThreads];
    getRequests(buffer, lineData);
    // printf("linedata[6].data is %s\n", lineData[6].data);
    createThreads(portNo, ipv4, lineData);

    return 0;
}

int openRequestFile(char *filePath){
    int requestFileFd;
    if((requestFileFd = open(filePath, O_RDONLY, 0666)) < 0)
        errorAndExit("Error while opening request file");
    return requestFileFd;
}

int getNumberOfRequests(int fileSize, int requestFd, char *buffer){
    if(read(requestFd, buffer, fileSize) < 0)
        errorAndExit("Error while reading request file");

    // READING NUMBER OF LINES/REQUESTS
    int numberOfLines = 0; // number of lines that contains transaction
    while(TRUE){
        char* index;
        if( (index = strstr(buffer, "transactionCount")) != NULL){ // return a pointer to after found
            numberOfLines++;
            buffer = index + 16;
        }else
            break;
    }
    printf("(%s) Number of lines is %d\n", timeStamp(), numberOfLines);
    return numberOfLines;
}

void getRequests(char *buffer, String *lineData){
    // Reading buffer with strtok
    char *line = strtok(buffer, "\n");
    for(int i = 0; i < s_numOfThreads && line != NULL; i++){
        // printf("strlen line is %ld\n", strlen(line));
        printf("line is %s\n", line);
        lineData[i].data = calloc(strlen(line) + 1, sizeof(char));
        strncpy(lineData[i].data, line, strlen(line)+1);
        if(buffer != NULL) line = strtok(NULL, "\n");
    }

    free(buffer);
}

void createThreads(int portNo, char *ipv4, String *lineData){
    // Initializing mutex and conditional variable
    pthread_mutex_init(&csMutex, NULL); 
    pthread_mutex_init(&barrierMutex, NULL); 
    pthread_cond_init(&barrierCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[s_numOfThreads];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // Creating threads
    for (int i = 0; i < s_numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, doClientJob, (void*)(&lineData[i])) != 0 ){ // if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
    }

    for(int i = 0; i < s_numOfThreads; i++){
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
    String *str = arg;
    printf("str is %s\n", str->data);

    pthread_exit(NULL);
}

void barrier(){
    pthread_mutex_lock(&barrierMutex);
    ++part1FinishedThreads;
    while(TRUE){ // Not busy waiting. Using conditional variable + mutex for monitoring
        if(part1FinishedThreads == s_numOfThreads /* Number of threads */){
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

