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
#include <arpa/inet.h> // Parser for socket connection
#include <netinet/in.h> // Socket connection
#include "../include/server.h"
#include "../include/common.h"

static int s_numOfConnectionsWaiting = 0;   // Number of connections waiting for server
static int s_numOfThreads, s_portNo;    // Number of threads and port number
static volatile sig_atomic_t didSigIntCome = 0; // sigint signal flag
static pthread_mutex_t csMutex;         // critical section mutex
static pthread_mutex_t adderMutex;    // Mutex for monitor
static pthread_cond_t monitorCond;      // Conditional variable for monitor thread
static SgLinkedList *s_connectionQueue; // Queue of coming connections
ServantSendingInfo *s_servantInfoList; // Holds information that initially received from servant (id, port, cities responsible of)

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
    pthread_mutex_init(&adderMutex, NULL); 
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

void *tcpComm(){ /* Job of main thread */
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
        if(didSigIntCome == 1 ){
            write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
            exit(EXIT_SUCCESS);
        }
        if (newServerSocketFd != -1 && newServerSocketFd != 0){ // if connection comes
            pthread_mutex_lock(&adderMutex); // Lock mutex 
            addToQueue(newServerSocketFd);  // As soon as it will be added to queue, child will take care of it.
            pthread_cond_signal(&monitorCond); // signal to condition variable in child thread to wake up for new connection
            pthread_mutex_unlock(&adderMutex); // Unlock mutex
        }
    }

    close(newServerSocketFd);
}

void *threadJob(void *arg){
    long threadId = (long)arg;
    dprintf(STDOUT_FILENO, "(%s) Thread-%ld is running and waiting for incoming connection\n", timeStamp(), threadId );

    // Using monitor for incoming connection from client/servant that's forwarded by main thread
    while(didSigIntCome != TRUE){
        pthread_mutex_lock(&csMutex);
        while(s_numOfConnectionsWaiting == 0){   // If no connection is waiting, wait for signal from main thread
            // printf("before cond\n");
            pthread_cond_wait(&monitorCond, &csMutex);
            // printf("After cond\n");
            if(didSigIntCome == TRUE){
                pthread_mutex_unlock(&csMutex);
                write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
                exit(EXIT_SUCCESS);
            }
        }

        // printf("Thread id-%ld is handling connection\n", threadId);
        int newServerSocketFd = removeFromQueue();
        pthread_mutex_unlock(&csMutex);
        // pthread_cond_signal(&monitorCond);
        // printf("Thread id-%ld is removed from queue\n", threadId);
        // pthread_mutex_unlock(&csMutex);
        handleIncomingConnection(newServerSocketFd);
        
    }
    
    pthread_exit(NULL);
}

void handleIncomingConnection(int newServerSocketFd){
    int clientOrServant; /* If request coming from client = 1, if request coming from servant = 2 */
    if(recv(newServerSocketFd, &clientOrServant, sizeof(int), 0) < 0){
        errorAndExit("Error receiving client/servant information");
    }
    if(clientOrServant == SERVANT)
        handleIncomingConnectionOfServant(newServerSocketFd);
    else if(clientOrServant == CLIENT)
        handleIncomingConnectionOfClient(newServerSocketFd);
    else
        errorAndExit("Error receiving client/servant information");   
}

void handleIncomingConnectionOfServant(int newServerSocketFd){
    // Receiving head and tail information from servant (Which sub-dataset it is responsible for)
    char firstCityName[25], lastCityName[25];
    ServantSendingInfo receivedInfoFromServant;
    if(recv(newServerSocketFd, &receivedInfoFromServant, sizeof(ServantSendingInfo), 0) < 0)
        errorAndExit("Error receiving head information");
    if(recv(newServerSocketFd, firstCityName, receivedInfoFromServant.cityName1Size, 0) < 0)
        errorAndExit("Error receiving first city name");
    // Receiving end city name as string
    if(recv(newServerSocketFd, lastCityName, receivedInfoFromServant.cityName2Size, 0) < 0)
        errorAndExit("Error receiving last city name");

    printf("(%s) Servant %d present at port %d and handling cities: %s-%s\n", timeStamp(), receivedInfoFromServant.procId, receivedInfoFromServant.portNoToUseLater, firstCityName, lastCityName);
    // Adding received information to linked list
    receivedInfoFromServant.cityName1 = firstCityName;
    receivedInfoFromServant.cityName2 = lastCityName;
    pthread_mutex_lock(&csMutex);
    addServantInfoToList(receivedInfoFromServant);
    pthread_mutex_unlock(&csMutex);
}

void handleIncomingConnectionOfClient(int newServerSocketFd){
    int dataSize;   char clientData[100];
    if(recv(newServerSocketFd, &dataSize, sizeof(int), 0) < 0)
        errorAndExit("Error receiving data size information");
    if(recv(newServerSocketFd, clientData, dataSize, 0) < 0)
        errorAndExit("Error receiving client data information");
    clientData[dataSize] = '\0';    // Putting end of string character at the end of the string to make it a valid string
    printf("(%s) Request arrived \"%s\"\n", timeStamp(), clientData);
    // For Ex. request is "transactionCount IMALATHANE 04-06-2004 11-11-2011 ISPARTA"
    // Parsing request
    char *token = strtok(clientData, " ");
    // int transactionCount = atoi(token);
    token = strtok(NULL, " ");
    char *estateType = token;
    token = strtok(NULL, " ");
    char *beginDate = token;
    token = strtok(NULL, " ");
    char *endDate = token;
    token = strtok(NULL, " ");
    char *cityName = token;
    if(cityName == NULL || strcmp(cityName, "") == 0 || strcmp(cityName, " ") == 0 ){
        // city name is empty so all servants will be asked to serve the request
        cityName = "-";
        printf("city is not provided\n");
        // çç
    }else{ // city name is not empty so only servants that are responsible for the city will be asked to serve the request
        // 1-find city name code in city list
        // 2-find servants that are responsible for the city
        // 3-give info to that servant with it's id
        printf("(%s) Parsing: Estate type is %s, begin %s end %s city %s\n", timeStamp(), estateType, beginDate, endDate, cityName);
        // find city name code in city list
        int responsibleServant = -1; 
        findResponsibleServant(cityName, &responsibleServant);
        if(responsibleServant == -1){
            // send error message to client (because responsible servant is -1)
            if(send(newServerSocketFd, &responsibleServant, sizeof(int), 0) < 0)
                errorAndExit("Error sending error message size");
            close(newServerSocketFd);
            return;
        }
        // Send info to corresponding servant to count transactions of that city
        int singleCityHandle = SINGLE_CITY_HANDLING;
        getTransactionCountFromServant(singleCityHandle, responsibleServant, estateType, beginDate, endDate, cityName);

    }

}

int getTransactionCountFromServant(int singleCityHandle, int responsibleServant, char *estateType, char *beginDate, char *endDate, char *cityName){
    // Send info to corresponding servant to count transactions of that city
    // use portnumber of that city
    int portNo;
    ServantSendingInfo *iter = s_servantInfoList;
    while(iter != NULL){
        if(iter->procId == responsibleServant){
            portNo = iter->portNoToUseLater;
            break;
        }
        iter = iter->next;
    }

    // Send info to servant with socket by creating new connection.
    int serverSocketFd, newSocketFdForThread;
    if((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errorAndExit("Error creating socket");
    }

    // Connecting to socket of servant
    struct sockaddr_in serverSocketAdressInfo;
    serverSocketAdressInfo.sin_family = AF_INET;
    printf("(%s) Server created a socket to handle servant comm with port no %d\n", timeStamp(), portNo);
    serverSocketAdressInfo.sin_port = htons(portNo);
    // serverSocketAdressInfo.sin_addr.s_addr = INADDR_ANY;
    char *s_ipv4 = "127.0.0.1"; // çç
    if (inet_pton(AF_INET, s_ipv4, &serverSocketAdressInfo.sin_addr) <= 0)
        errorAndExit("Inet pton error with ip adress.\n");

    if( (newSocketFdForThread = connect(serverSocketFd, (struct sockaddr *)&serverSocketAdressInfo, sizeof(serverSocketAdressInfo))) < 0){
        errorAndExit("Error connecting to socket");
    }

    // This loop is not busy waiting because waits until get accepted.
    while (TRUE){
        if(didSigIntCome == 1 ){
            write(STDOUT_FILENO, "Successfully exiting with SIGINT\n", 33);
            exit(EXIT_SUCCESS);
        }
        // Parsing dates            
        int beginDateArray[3];
        int endDateArray[3];

        dateParser(beginDate, beginDateArray);
        dateParser(endDate, endDateArray);

        // sending info to servant
        ServantGettingInfo *servantGettingInfo = malloc(sizeof(*servantGettingInfo) + strlen(estateType) + strlen(cityName) + 2);
        servantGettingInfo->cityHandleType = SINGLE_CITY_HANDLING;
        servantGettingInfo->beginDay = beginDateArray[0];
        servantGettingInfo->beginMonth = beginDateArray[1];
        servantGettingInfo->beginYear = beginDateArray[2];
        servantGettingInfo->endDay = endDateArray[0];
        servantGettingInfo->endMonth = endDateArray[1];
        servantGettingInfo->endYear = endDateArray[2];

        servantGettingInfo->structSize = sizeof(*servantGettingInfo) + strlen(estateType) + strlen(cityName) + 2;
        // Merging estate type and city name to one string (because fam can have only one element in struct)
        strncpy(servantGettingInfo->estateTypeAndCity, estateType, strlen(estateType));
        strcpy(servantGettingInfo->estateTypeAndCity + strlen(estateType), "\n");
        strncpy(servantGettingInfo->estateTypeAndCity + strlen(estateType) + 1, cityName, strlen(cityName));
        strncpy(servantGettingInfo->estateTypeAndCity + strlen(estateType) + strlen(cityName) + 1, "\0", 1);

        if(send(serverSocketFd, &servantGettingInfo->structSize, sizeof(int), 0) < 0)
            errorAndExit("Error sending size info");

        if(send(serverSocketFd, servantGettingInfo, servantGettingInfo->structSize, 0) < 0)
            errorAndExit("Error sending servant info");
        // Receive info from servant
        ServantGettingInfo receivedInfoFromServant;
        if(recv(serverSocketFd, &receivedInfoFromServant, sizeof(ServantGettingInfo), 0) < 0)
            errorAndExit("Error receiving servant info");
    }

    close(newSocketFdForThread);   

}

void dateParser(char *date, int *dateArray){
    char *token = strtok(date, "-");
    int i = 0;
    while(token != NULL){
        dateArray[i] = atoi(token);
        token = strtok(NULL, "-");
        i++;
    }
}

void findResponsibleServant(char *cityName, int *responsibleServant){
    int found = FALSE;
    // check every servants info in list and find the city code and responsible servant
    // checking s_servantInfoList to find about the responsible servant for the city with comparing cityName with cityName1 and cityName2
    ServantSendingInfo *currentServantInfo = s_servantInfoList;
    if(currentServantInfo == NULL)
        return;
    while(currentServantInfo != NULL || found == TRUE){
        // printf("currenservant->procid is %d\n", currentServantInfo->procId);
        // printf("currentServantInfo->cityName1 is %s\n", currentServantInfo->cityName1);
        // printf("currentServantInfo->cityName2 is %s\n", currentServantInfo->cityName2);
        if(strncmp(currentServantInfo->cityName1, cityName, strlen(currentServantInfo->cityName1) ) <= 0
        && strncmp(currentServantInfo->cityName2, cityName, strlen(currentServantInfo->cityName2) ) >= 0){
            *responsibleServant = currentServantInfo->procId;
            // printf("(%s) Responsible servant founded the city \"%s\" and it's servant number %d\n", timeStamp(), cityName, currentServantInfo->procId);
            printf("(%s) Contacting servant %d", timeStamp(), currentServantInfo->procId);
            found = TRUE; break;
        }
        currentServantInfo = currentServantInfo->next;
    }
    if(found == FALSE){
        printf("(%s) Responsible servant couldn't found the city \"%s\" in its data structure\n", timeStamp(), cityName);
    }

}

void addServantInfoToList(ServantSendingInfo receivedInfoFromServant){ 
    ServantSendingInfo *temp = NULL;
    // Adding received information to linked list
    if(s_servantInfoList == NULL){
        s_servantInfoList = (ServantSendingInfo *)malloc(sizeof(ServantSendingInfo));
        s_servantInfoList->next = NULL;
        temp = s_servantInfoList;
    }
    else{
        temp = s_servantInfoList;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = (ServantSendingInfo *)malloc(sizeof(ServantSendingInfo));
        temp = temp->next;
    }

    temp->head = receivedInfoFromServant.head;
    temp->tail = receivedInfoFromServant.tail;
    temp->portNoToUseLater = receivedInfoFromServant.portNoToUseLater;
    temp->procId = receivedInfoFromServant.procId;

    temp->cityName1 = malloc(strlen(receivedInfoFromServant.cityName1) + 1);
    temp->cityName2 = malloc(strlen(receivedInfoFromServant.cityName2) + 1);
    strncpy(temp->cityName1, receivedInfoFromServant.cityName1, strlen(receivedInfoFromServant.cityName1));
    strncpy(temp->cityName2, receivedInfoFromServant.cityName2, strlen(receivedInfoFromServant.cityName2));
    temp->cityName1[strlen(receivedInfoFromServant.cityName1)] = '\0';
    temp->cityName2[strlen(receivedInfoFromServant.cityName2)] = '\0';

    temp->cityName1Size = receivedInfoFromServant.cityName1Size;
    temp->cityName2Size = receivedInfoFromServant.cityName2Size;
    temp->next = NULL;

    // printf("Servant info list adress is %p\n", s_servantInfoList);
    // printf("Received info->cityname1 %s temp->city %s\n", receivedInfoFromServant.cityName1, temp->cityName1);
    // printf("(%s) Server: Added to list s_servant->name %s\n", timeStamp(), s_servantInfoList->cityName1);
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