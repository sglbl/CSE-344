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
#include <dirent.h> // Directory and file
#include <arpa/inet.h> // Parser for ip adress
#include "../include/server.h"
#include "../include/common.h"

static int numOfThreads;
static volatile sig_atomic_t didSigIntCome = 0;
static pthread_mutex_t csMutex;
static pthread_mutex_t barrierMutex;
static pthread_cond_t barrierCond;
static int part1FinishedThreads;

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    int option, portNo = -1, noOfThreads = -1;

    while ((option = getopt(argc, argv, "p:t:")) != -1){
        switch (option) {
            case 'p': // there will be c times consumer threads
                portNo = atoi(optarg);
                break;
            case 't':
                noOfThreads = atoi(optarg);
                break;
            default:
                write(STDERR_FILENO, "Error with arguments\nUsage: ./server -p PORT -t numberOfThreads\n", 64);
                exit(EXIT_FAILURE);
        }
    }

    if( portNo <= 2000 || noOfThreads < 5 ){
        write(STDERR_FILENO, "Error. It should be p > 2000, t >= 5\nUsage: ./server -p PORT -t numberOfThreads\n", 80);
        exit(EXIT_FAILURE);
    }

    signalHandlerInitializer();
    // int fileDescs[3];
    // openFiles(filePath1, filePath2, outputPath, fileDescs);
    numOfThreads = noOfThreads;
    getSocketInfoFromServant(portNo);
    // createThreads(portNo);

    return 0;
}

void getSocketInfoFromServant(int portNo){
    // Create socket
    int serverSocketFdToGetFromServant;
    if((serverSocketFdToGetFromServant = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errorAndExit("Error creating socket");
    }

    // Connecting to socket of servant
    struct sockaddr_in serverSocketInfo;
    serverSocketInfo.sin_family = AF_INET;
    serverSocketInfo.sin_port = htons(portNo);
    serverSocketInfo.sin_addr.s_addr = INADDR_ANY;
    if(connect(serverSocketFdToGetFromServant, (struct sockaddr *)&serverSocketInfo, sizeof(serverSocketInfo)) < 0){
        errorAndExit("Error connecting to socket");
    }
    else{
        printf("Success\n");
        // Receiving head and tail information from servant 
        // (Which sub-dataset it is responsible for)
        int head, tail, portNoToConnectTo;
        if(recv(serverSocketFdToGetFromServant, &head, sizeof(int), 0) < 0){
            errorAndExit("Error receiving head");
        }
        if(recv(serverSocketFdToGetFromServant, &tail, sizeof(int), 0) < 0){
            errorAndExit("Error receiving tail");
        }
        // get from which port number the server can connect to this servant after.
        if(recv(serverSocketFdToGetFromServant, &portNoToConnectTo, sizeof(int), 0) < 0){
            errorAndExit("Error receiving port number");
        }

        printf("Head: %d, Tail: %d, portNoToConnect %d\n", head, tail, portNoToConnectTo);
    }

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

void createThreads(int portNo){
    // Initializing mutex and conditional variable
    pthread_mutex_init(&csMutex, NULL); 
    pthread_mutex_init(&barrierMutex, NULL); 
    pthread_cond_init(&barrierCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[numOfThreads];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    typedef int Info; //çç
    Info info[numOfThreads];
    memset(info, 0, sizeof(info));

    // Creating threads
    for (int i = 0; i < numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, threadJob, (void*)&info[i]) != 0 ){ // if returns 0 it's okay.
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

void *threadJob(void *arg){
    // Info *infoArg = arg;
    setbuf(stdout, NULL);

    dprintf(STDOUT_FILENO, "(%s) Thread is running\n", timeStamp() );

    // Barrier    
    barrier();

    // Part2
    
   pthread_exit(NULL);
}

void barrier(){
    pthread_mutex_lock(&barrierMutex);
    ++part1FinishedThreads;
    while(TRUE){ // Not busy waiting. Using conditional variable + mutex for monitoring
        if(part1FinishedThreads == numOfThreads /* Number of threads */){
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
