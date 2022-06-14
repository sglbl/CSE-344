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
static pthread_mutex_t monitorMutex;
static pthread_cond_t monitorCond;
static int part1FinishedThreads;
//queue to store the incoming connections and distribute them to non-busy pool of threads
static int s_numOfConnectionsWaiting = 0;
static SgLinkedList *s_connectionQueue;
static SgLinkedList *frontOfQueue = NULL, *rearOfQueue = NULL;

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
    s_numOfThreads = noOfThreads;
    s_portNo = portNo;
    createThreads(portNo);
    tcpComm();

    return 0;
}

void createThreads(int portNo){
    // Initializing mutex and conditional variable
    pthread_mutex_init(&csMutex, NULL); 
    pthread_mutex_init(&monitorMutex, NULL); 
    pthread_cond_init(&monitorCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[s_numOfThreads];

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // typedef int Info; //çç
    // Info info[s_numOfThreads];
    // memset(info, 0, sizeof(info));

    // // Creating threads
    // for (int i = 0; i < s_numOfThreads; i++){  
    //     if( pthread_create(&threads[i], &attr, threadJob, (void*)&info[i]) != 0 ){ // if returns 0 it's okay.
    //         errorAndExit("pthread_create()");
    //     }
    // }

    for (int i = 0; i < s_numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, threadJob, NULL) != 0 ){
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
    dprintf(STDOUT_FILENO, "(%s) Thread-%d is running\n", timeStamp(), pthread_self() );

    // Using monitor for incoming connection from client
    while(TRUE){
        pthread_mutex_lock(&monitorMutex);
        if( s_numOfConnectionsWaiting == 0 ){
            pthread_cond_wait(&monitorCond, &monitorMutex);
        }
        s_numOfConnectionsWaiting--;
        pthread_mutex_unlock(&monitorMutex);
    }
    
    pthread_exit(NULL);
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

    // This loop is not busy waiting because waits until get accepted.
    while (TRUE){
        socklen_t addresslength = sizeof(address);
        char firstCityName[25], lastCityName[25];
        // Wait until a client/servant connects
        if( (newServerSocketFd = accept(serverSocketFd, (struct sockaddr *)&address, &addresslength)) < 0)
            errorAndExit("Accept error\n");
        if (newServerSocketFd != -1 && newServerSocketFd != 0){
            // Add request to jobs çç
            // addToQueue(newServerSocketFd);

            // forwardIncomingConnection(newServerSocketFd);

            // removeFromQueue(newServerSocketFd);

            pthread_mutex_lock(&csMutex);
            // add_request(serverSocket);
            printf("(%s) Server: Accepted\n", timeStamp());

            // Receiving head and tail information from servant 
            // (Which sub-dataset it is responsible for)
            int clientOrServant; /* If request coming from client = 1, if request coming from servant = 2 */
            if(recv(newServerSocketFd, &clientOrServant, sizeof(int), 0) < 0){
                errorAndExit("Error receiving client/servant information");
            }
            if(clientOrServant == SERVANT){
                ServantSendingInfo receivedInfoFromServant;
                if(recv(newServerSocketFd, &receivedInfoFromServant, sizeof(ServantSendingInfo), 0) < 0){
                    errorAndExit("Error receiving head information");
                }
                if(recv(newServerSocketFd, firstCityName, receivedInfoFromServant.cityName1Size, 0) < 0)
                    errorAndExit("Error receiving first city name");
                // Receiving end city name as string
                if(recv(newServerSocketFd, lastCityName, receivedInfoFromServant.cityName2Size, 0) < 0)
                    errorAndExit("Error receiving last city name");

                printf("Server got: %d %d %d and got cities: %s %s\n", receivedInfoFromServant.head, receivedInfoFromServant.tail, receivedInfoFromServant.portNoToUseLater, firstCityName, lastCityName);
            }
            else if(clientOrServant == CLIENT){
                int dataSize;
                if(recv(newServerSocketFd, &dataSize, sizeof(int), 0) < 0)
                    errorAndExit("Error receiving head information");
                if(recv(newServerSocketFd, data, dataSize, 0) < 0)
                    errorAndExit("Error receiving head information");
                 // Putting end of string character at the end of the string
                data[dataSize] = '\0';
                printf("(%s) Request arrived \"%s\"\n", timeStamp(), data);
            }
            else
                errorAndExit("Error receiving client/servant information");
            
            pthread_mutex_unlock(&csMutex);
            // pthread_cond_signal(&cond_job); // Signal new job çç
        }
    }

    close(newServerSocketFd);

}

void forwardIncomingConnection(int newServerSocketFd){
    // Forwarding the connection to the next available thread
    int i;
    for(i = 0; i < s_numOfThreads; i++){
        if(!threads[i].isBusy){
            threads[i].isBusy = 1;
            threads[i].socketFd = newServerSocketFd;
            break;
        }
    }
    if(i == s_numOfThreads)
        errorAndExit("No thread available");
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

// int checkWhichThreadIsNotBusy(){
//     SgLinkedList *iterator = s_connectionQueue;
//     while(iterator->next != NULL){

//     }



//     if(threads[threadId].isBusy == 0)
//         errorAndExit("Thread is not busy");
// }

void addToQueue(int newFileDesc){
    pthread_mutex_lock(&csMutex);
    // Creating a new SgLinkedList element
    SgLinkedList *newElement = (SgLinkedList*)malloc(sizeof(SgLinkedList));
    newElement->fileDesc = newFileDesc;
    newElement->next = NULL;
    if(s_connectionQueue == NULL){
        s_connectionQueue = newElement;
    }
    else{
        SgLinkedList *temp = s_connectionQueue;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = newElement;
    }
    // incrementing the number of connections
    ++s_numOfConnectionsWaiting;
    // Value added to queue. Now we can unlock
    pthread_mutex_unlock(&csMutex);
}

void removeFromQueue(int newFileId){
    pthread_mutex_lock(&csMutex);
    SgLinkedList *prev = NULL;
    SgLinkedList *temp = s_connectionQueue;
    while(temp != NULL){
        if(temp->fileDesc == newFileId){
            if(prev == NULL){
                s_connectionQueue = temp->next;
            }
            else{
                prev->next = temp->next;
            }
            free(temp);
            s_numOfConnectionsWaiting--;
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    --s_numOfConnectionsWaiting;
    pthread_mutex_unlock(&csMutex);
}