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

static int s_numOfConnectionsWaiting = 0;
static int s_numOfThreads, s_portNo;
static volatile sig_atomic_t didSigIntCome = 0;
static pthread_mutex_t csMutex;
static pthread_mutex_t monitorMutex;
static pthread_cond_t monitorCond;
static SgLinkedList *s_connectionQueue;

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

    return 0;
}

void createThreads(int portNo){
    // Initializing mutex and conditional variable
    pthread_mutex_init(&csMutex, NULL); 
    // pthread_mutex_init(&monitorMutex, NULL); 
    pthread_cond_init(&monitorCond, NULL);
    pthread_attr_t attr;
    pthread_t threads[s_numOfThreads];
    pthread_t mainThread;

    // set global thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    
    // Starting tcp stream socket communication as server for main thread
    if( pthread_create(&mainThread, NULL, tcpComm, (void*)0) != 0 ){ //if returns 0 it's okay.
            errorAndExit("pthread_create()");
    }

    // Creating threads
    for (int i = 0; i < s_numOfThreads; i++){  
        if( pthread_create(&threads[i], &attr, threadJob, (void*)(long)i) != 0 ){ // if returns 0 it's okay.
            errorAndExit("pthread_create()");
        }
    }

    for(int i = 0; i < s_numOfThreads; i++){
        if( pthread_join(threads[i], NULL) != 0 ){
            errorAndExit("pthread_join()");
        }
    }

    if( pthread_detach(mainThread) != 0 ){ // main thread is detached
        errorAndExit("pthread_detach() error for main thread");
    }

    if(didSigIntCome == 1 ){
        write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
        exit(EXIT_SUCCESS);
    }

    // Freeing allocated memory of matrix C and output matrix
    exit(EXIT_SUCCESS);
}

void *tcpComm(){
    printf("(%s) Server is waiting\n", timeStamp());
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
        if(didSigIntCome == 1 ){
            write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
            exit(EXIT_SUCCESS);
        }
        socklen_t addresslength = sizeof(address);
        // Wait until a client/servant connects
        if( (newServerSocketFd = accept(serverSocketFd, (struct sockaddr *)&address, &addresslength)) < 0)
            errorAndExit("Accept error\n");
        if (newServerSocketFd != -1 && newServerSocketFd != 0){ // if connection comes
            pthread_mutex_lock(&csMutex); // Lock mutex 
            addToQueue(newServerSocketFd);  // As soon as it will be added to queue, child will take care of it.
            pthread_cond_signal(&monitorCond); // signal to condition variable in child thread to wake up for new connection
            pthread_mutex_unlock(&csMutex); // Unlock mutex
        }
    }

    close(newServerSocketFd);
}

void *threadJob(void *arg){
    long threadId = (long)arg;
    dprintf(STDOUT_FILENO, "(%s) Thread-%ld is running and waiting for incoming connection\n", timeStamp(), threadId );

    // Using monitor for incoming connection from client/servant that's forwarded by main thread
    while(didSigIntCome != TRUE){
        dprintf(STDOUT_FILENO, "Thread-%ld is waiting for incoming connection\n", threadId);
        if( s_numOfConnectionsWaiting == 0 ){   // If no connection is waiting, wait for signal from main thread
            pthread_cond_wait(&monitorCond, &monitorMutex);
        }else{ // there is a connection waiting in queue
            pthread_mutex_lock(&csMutex);
            int newServerSocketFd = removeFromQueue();
            pthread_mutex_unlock(&csMutex);
            handleIncomingConnection(newServerSocketFd);
        }
    }
    
    pthread_exit(NULL);
}

void handleIncomingConnection(int newServerSocketFd){
    printf("(%s) Server: Accepted\n", timeStamp());

    // Receiving head and tail information from servant 
    // (Which sub-dataset it is responsible for)
    char firstCityName[25], lastCityName[25], clientData[100];
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
        if(recv(newServerSocketFd, clientData, dataSize, 0) < 0)
            errorAndExit("Error receiving head information");
            // Putting end of string character at the end of the string
        clientData[dataSize] = '\0';
        printf("(%s) Request arrived \"%s\"\n", timeStamp(), clientData);
    }
    else
        errorAndExit("Error receiving client/servant information");   
}

static void exitingJob(){
    //çç free and close
    printf("Exiting job is signaling all to exit");
    // send broadcast signal to end all
    pthread_mutex_lock(&csMutex);
    pthread_cond_broadcast(&monitorCond);
    pthread_mutex_unlock(&csMutex);
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

void addToQueue(int newFileDesc){
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
}

// void removeFromQueue(int newFileId){
//     pthread_mutex_lock(&csMutex);
//     SgLinkedList *prev = NULL;
//     SgLinkedList *temp = s_connectionQueue;
//     while(temp != NULL){
//         if(temp->fileDesc == newFileId){
//             if(prev == NULL){
//                 s_connectionQueue = temp->next;
//             }
//             else{
//                 prev->next = temp->next;
//             }
//             free(temp);
//             s_numOfConnectionsWaiting--;
//             break;
//         }
//         prev = temp;
//         temp = temp->next;
//     }
//     --s_numOfConnectionsWaiting;
//     pthread_mutex_unlock(&csMutex);
// }

int removeFromQueue(){
    // remove first element from queue
    // pthread_mutex_lock(&csMutex);
    SgLinkedList *temp = s_connectionQueue;
    // removing first element and returning its file descriptor
    int fileDesc = temp->fileDesc;
    s_connectionQueue = temp->next;
    free(temp);
    --s_numOfConnectionsWaiting;
    // pthread_mutex_unlock(&csMutex);
    return fileDesc;
}