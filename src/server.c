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

static int s_numOfThreads, s_portNo;
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
    s_numOfThreads = noOfThreads;
    s_portNo = portNo;
    // getSocketInfoFromServant(portNo);
    tcpComm();
    // createThreads(portNo);

    return 0;
}

void tcpComm(){
    printf("(%s) Server is waiting\n", timeStamp());
    char data[100];
    struct sockaddr_in address;
    struct sockaddr_in serverSocketAdress;
    int serverSocketFd, newServerSocketFd;
    if ((serverSocketFd = socket(AF_INET/*ipv4*/, SOCK_STREAM, 0)) == -1) {
        errorAndExit("Socket creation error\n");
    }
    
    serverSocketAdress.sin_family = AF_INET;
    serverSocketAdress.sin_port = htons(s_portNo);
    serverSocketAdress.sin_addr.s_addr = INADDR_ANY;

    if ((bind(serverSocketFd, (struct sockaddr *)&serverSocketAdress, sizeof(serverSocketAdress))) == -1)
        errorAndExit("Bind error");

    if ((listen(serverSocketFd, 256 /*256 connections will be queued*/)) == -1)
        errorAndExit("Listen error");

    while (TRUE){
        socklen_t addresslength = sizeof(address);
        // printf("Busy waiting checker\n"); // Not busy waiting because waits until get accepted.
        if( (newServerSocketFd = accept(serverSocketFd, (struct sockaddr *)&address, &addresslength)) < 0){
            errorAndExit("Accept error\n");
        }
        if (newServerSocketFd > 0){
            // Add request to jobs çç
            pthread_mutex_lock(&csMutex);
            // add_request(serverSocket);
            printf("Accepted\n\n");

            // Receiving head and tail information from servant 
            // (Which sub-dataset it is responsible for)
            int clientOrServant; /* If request coming from client = 1, if request coming from servant = 2 */
            int head, tail, portNoToConnectTo;
            if(recv(newServerSocketFd, &clientOrServant, sizeof(int), 0) < 0){
                errorAndExit("Error receiving client/servant information");
            }
            if(clientOrServant == SERVANT){
                // printf("(%s) Servant %d present at port %d handling cities %s-%s\n", timeStamp()); //çç
                if(recv(newServerSocketFd, &head, sizeof(int), 0) < 0){
                    errorAndExit("Error receiving head information");
                }
                if(recv(newServerSocketFd, &tail, sizeof(int), 0) < 0){
                    errorAndExit("Error receiving tail information");
                }
                if(recv(newServerSocketFd, &portNoToConnectTo, sizeof(int), 0) < 0){
                    errorAndExit("Error receiving portNoToConnectTo information");
                }
                printf("Head: %d, Tail: %d, portNoToConnect %d\n", head, tail, portNoToConnectTo);
            }
            else if(clientOrServant == CLIENT){
                int dataSize;
                if(recv(newServerSocketFd, &dataSize, sizeof(int), 0) < 0){
                    errorAndExit("Error receiving head information");
                }
                printf("Data size: %d\n", dataSize);
                if(recv(newServerSocketFd, data, dataSize, 0) < 0){
                    errorAndExit("Error receiving head information");
                }
                data[dataSize] = '\0'; // Putting end of string character at the end of the string
                printf("(%s) Request arrived \"%s\"\n", timeStamp(), data);
            }
            else{
                errorAndExit("Error receiving client/servant information");
            }
            
            pthread_mutex_unlock(&csMutex);
            // pthread_cond_signal(&cond_job); // Signal new job çç
        }
    }

    close(newServerSocketFd);

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
    pthread_t threads[s_numOfThreads];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    typedef int Info; //çç
    Info info[s_numOfThreads];
    memset(info, 0, sizeof(info));

    // Creating threads
    for (int i = 0; i < s_numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, threadJob, (void*)&info[i]) != 0 ){ // if returns 0 it's okay.
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
