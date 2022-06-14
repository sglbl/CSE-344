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
#include <sys/procfs.h> // Process information
#include "../include/servant.h"
#include "../include/common.h"

static volatile sig_atomic_t didSigIntCome = 0;
static pthread_mutex_t csMutex;
int serverSocket;

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    int option, portNo = -1;
    char *dirPath, *citiesToHandle, *ipv4;

    while ((option = getopt(argc, argv, "d:c:r:p:")) != -1){
        switch (option){
            case 'd':
                dirPath = optarg;
                break;
            case 'c':
                citiesToHandle = optarg;
                break;
            case 'r':
                ipv4 = optarg;
                break;
            case 'p':
                portNo = atoi(optarg);
                break;
            default:
                write(STDERR_FILENO, "Error with arguments\nUsage: ./servant -d directoryPath -c 10-19 -r IP -p PORT\n", 78);
                exit(EXIT_FAILURE);
        }
    }

    if( portNo <= 2000){
        write(STDERR_FILENO, "Error. It should be p > 2000, t >= 5\nUsage: ./servant -d directoryPath -c 10-19 -r IP -p PORT\n", 94);
        exit(EXIT_FAILURE);
    }

    signalHandlerInitializer();
    pthread_mutex_init(&csMutex, NULL); // Initialize mutex
    doServantJob(dirPath, citiesToHandle, ipv4, portNo);

    return 0;
}

void doServantJob(char *dirPath, char *citiesToHandle, char *ipv4Adress, int portNo){
    // Open the folders in dataset folder with alphabetic numbers from 10 to 19

    printf("Servant is working on %s\n", dirPath);
    int head = 0, tail = 0;
    cityQueueParser(citiesToHandle, &head, &tail);

    SgCityLinkedList *cityList = calloc(1, sizeof(SgCityLinkedList));
    cityList->next = NULL;

    // READING PROVINCE NAMES FROM THE DATASET FOLDER
    struct dirent **allTheEntities;
    int readed;
    if((readed = scandir(dirPath, &allTheEntities, NULL, alphasort)) < 0)
        errorAndExit("scandir error while searching for directories");
    for(int i = 0; i < readed; ++i){
        if(i - 1 >= head && i - 1 <= tail){
            cityList = addCityToLinkedList(cityList, allTheEntities[i]->d_name);
        }
        free(allTheEntities[i]);
    }
    free(allTheEntities);

    // READING FILE NAMES IN THE PROVINCE FOLDERS
    DIR *dir;
    struct dirent *ent;
    SgCityLinkedList *cityIter = cityList;

    while(cityIter != NULL && cityIter->cityName.data != NULL){
        cityIter->dateLL = calloc(1, sizeof(SgDateLinkedList));
        cityIter->dateLL->next = NULL;
        // printf("iter->data (city name): %s\n", cityIter->cityName.data);

        char *cityDirPath = malloc(strlen(dirPath) + strlen(cityIter->cityName.data) + 2);
        strcpy(cityDirPath, dirPath);
        strcat(cityDirPath, "/");
        strcat(cityDirPath, cityIter->cityName.data);

        if((dir = opendir(cityDirPath)) == NULL)
            errorAndExit("opendir error while opening directory");
        while((ent = readdir(dir)) != NULL){
            if(strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0){
                // printf("entDname %s\n", ent->d_name);
                cityIter->dateLL = addDateToLinkedList(cityIter->dateLL, ent->d_name);
                cityIter->dateLL->transactions = calloc(1, sizeof(SgLinkedList));
                cityIter->dateLL->transactions->next = NULL;
                readFileOfTranscations(cityIter->dateLL->transactions, cityDirPath/*City directory*/, ent->d_name/*date*/);
            }
        }
        closedir(dir);
        cityIter = cityIter->next;
    }

    // printDateLinkedList(cityList);
    SgCityLinkedList *iter = cityList;
    char *cityName1 = iter->cityName.data;
    for(int i = 0; i < tail - head; i++)
        iter = iter->next;
    char *cityName2 = iter->cityName.data;

    printf("(%s) Servant-%d: Loaded dataset, cities %s-%s\n", timeStamp(), getPidWithProp(), cityName1, cityName2);
    servantTcpCommWithServer(cityList, ipv4Adress, portNo, head, tail);
}

void readFileOfTranscations(SgLinkedList *transactions, char *cityDirPath, char *date){
    char *filePath = malloc(strlen(cityDirPath) + strlen(date) + 2);
    strcpy(filePath, cityDirPath);
    strcat(filePath, "/");
    strcat(filePath, date);

    // Reading file with read()
    int transactionFd;
    if((transactionFd = open(filePath, O_RDONLY, 0666)) < 0)
        errorAndExit("Error while opening request file");

    // Reading number of lines in the request file by using stat
    struct stat statOfFile;
    stat(filePath, &statOfFile);
    // Buffer will be used to temporarily storing.
    char *buffer = calloc( statOfFile.st_size , 1);

    if(read(transactionFd, buffer, statOfFile.st_size) < 0)
        errorAndExit("error while reading file");

    // Parsing file with strtok()
    char *line = strtok(buffer, "\n");
    for(int i = 0; line != NULL || line[0] != '\0'; i++){
        transactions = addTransactionToLinkedList(transactions, line);
        line = strtok(NULL, "\n");
        if(line == NULL)    break;
    }

}

void cityQueueParser(char *citiesToHandle, int *head, int *tail){
    // String token with :
    char *token = strtok(citiesToHandle, "-");
    for(int i = 0; token != NULL; i++){
        if(i == 0)      *head = atoi(token);
        else if(i == 1) *tail = atoi(token);
        token = strtok(NULL, ":");
    }
}

void servantTcpCommWithServer(SgCityLinkedList *cityList, char *ipv4Adress, int portNo, int head, int tail){
    // Create socket
    int serverSocketFd, servantSocketFd;
    if((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errorAndExit("Error creating socket");
    }

    int procId = getPidWithProp();

    // Connecting to socket of servant
    struct sockaddr_in serverSocketAdressInfo;
    serverSocketAdressInfo.sin_family = AF_INET;
    serverSocketAdressInfo.sin_port = htons(portNo);
    // serverSocketAdressInfo.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, ipv4Adress, &serverSocketAdressInfo.sin_addr) <= 0) 
        errorAndExit("Inet pton error with ip adress.\n");
    if( (servantSocketFd = connect(serverSocketFd, (struct sockaddr *)&serverSocketAdressInfo, sizeof(serverSocketAdressInfo))) < 0)
        errorAndExit("Error connecting to socket");

    while (TRUE){
        pthread_mutex_lock(&csMutex);
        int iAmServant = SERVANT;
        printf("(%s) Servant-%d: Informing server that I am servant\n", timeStamp(), procId);
        if(send(serverSocketFd, &iAmServant, sizeof(int), 0) == -1)
            errorAndExit("Error sending client info\n");

        // Sending information of servant to the server
        ServantSendingInfo sendingInfo;
        char *cityName1 = cityList->cityName.data;
        for(int i = 0; i < tail - head; i++)
            cityList = cityList->next;
        char *cityName2 = cityList->cityName.data;

        sendingInfo.head = head; 
        sendingInfo.tail = tail;
        sendingInfo.procId = procId;
        // Sending city name sizes with +1 because of '\0' character
        sendingInfo.cityName1Size = strlen(cityName1) + 1;
        sendingInfo.cityName2Size = strlen(cityName2) + 1;

        // Sending portNo that will be used later
        ++portNoThatServantIsListeningOn;
        int portNoToUseLater = portNoThatServantIsListeningOn;
        sendingInfo.portNoToUseLater = portNoToUseLater;

        if(send(serverSocketFd, &sendingInfo, sizeof(ServantSendingInfo), 0) == -1)
            errorAndExit("Error while sending data");

        // send first city name that this servant is handling
        if(send(serverSocketFd, cityName1, sendingInfo.cityName1Size, 0) == -1)
            errorAndExit("Error sending data\n");
        // send last city name that this servant is handling
        if(send(serverSocketFd, cityName2, sendingInfo.cityName2Size, 0) == -1)
            errorAndExit("Error sending data\n");

        
        pthread_mutex_unlock(&csMutex);

        // RECEIVING RESPONSE FROM SERVER
        int response;
        if(recv(serverSocketFd, &response, sizeof(int), 0) < 0)
            errorAndExit("Error receiving head information");
    }

    close(servantSocketFd);
}

int getPidWithProp(){
    // Instead pidof using proc/self/status with open() system call
    int procIdFd = open("/proc/self/status", O_RDONLY);
    if(procIdFd < 0)
        errorAndExit("Error opening /proc/self/status");

    // Reading /proc/self/status
    char *buffer = malloc(100);
    if(read(procIdFd, buffer, 100) < 0)
        errorAndExit("Error reading /proc/self/status");
    // printf("buffer is %s\n", buffer);

    // Finding "Pid:\t" in file with strstr
    char *pid, *ppid;
    if((pid = strstr(buffer, "Pid:\t")) == NULL)
        errorAndExit("Error finding pid in /proc/self/status");
    pid += 5;
    // Finding "\nPPid:" in file with strstr
    if((ppid = strstr(buffer, "\nPPid:")) == NULL)
        errorAndExit("Error finding ppid in /proc/self/status");
    ppid[0] = '\0';

    int pidInteger = atoi(pid);    
    free(buffer);

    return pidInteger;
}

SgDateLinkedList *addDateToLinkedList(SgDateLinkedList *head, char *date){
    SgDateLinkedList *tempIterator = head;
    while(tempIterator->next != NULL){
		tempIterator = tempIterator->next;	
	}
	tempIterator->next=(SgDateLinkedList*)calloc(1, sizeof(SgDateLinkedList));
    int stringLength = strlen(date);
    tempIterator->date.data = calloc(stringLength + 1, sizeof(char));
    strncpy(tempIterator->date.data, date, stringLength);
    tempIterator->date.data[stringLength] = '\0';

	tempIterator->next->next=NULL;

    return head;    
}

SgCityLinkedList *addCityToLinkedList(SgCityLinkedList *head, char *cityName){
    SgCityLinkedList *tempIterator = head;
    while(tempIterator->next != NULL){
		tempIterator = tempIterator->next;	
	}
	tempIterator->next=(SgCityLinkedList*)calloc(1, sizeof(SgCityLinkedList));
    int stringLength = strlen(cityName);
    tempIterator->cityName.data = calloc(stringLength + 1, sizeof(char));
    strncpy(tempIterator->cityName.data, cityName, stringLength);
    tempIterator->cityName.data[stringLength] = '\0';

	tempIterator->next->next=NULL;

    return head;
}

SgLinkedList *addTransactionToLinkedList(SgLinkedList *head, char *transaction){
    SgLinkedList *tempIterator = head;
    while(tempIterator->next != NULL){
		tempIterator = tempIterator->next;	
	}
	tempIterator->next=(SgLinkedList*)calloc(1, sizeof(SgLinkedList));
    int stringLength = strlen(transaction);
    tempIterator->string.data = calloc(stringLength + 1, sizeof(char));
    strncpy(tempIterator->string.data, transaction, stringLength);
    tempIterator->string.data[stringLength] = '\0';

	tempIterator->next->next=NULL;
    return head;
}

void printDateLinkedList(SgCityLinkedList *iter){
    printf("(%s) Printing linkedlist values\n", timeStamp());
    while(iter->next != NULL){
        printf("(%s) %s\n", timeStamp(), iter->cityName.data);
        while(iter->dateLL->next != NULL){
            printf("(%s) %s\n", timeStamp(), iter->dateLL->date.data);
            iter->dateLL = iter->dateLL->next;
        }
        iter = iter->next;
    }
}

void printLinkedList(SgLinkedList *iter){
    printf("(%s) Printing linkedlist values\n", timeStamp());
    while(iter->next != NULL){
        printf("(%s) %s\n", timeStamp(), iter->string.data);
        iter = iter->next;
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
    
    char* string = calloc(digitCounter + 1, sizeof(char) );
    for(int i = 0; i < digitCounter; i++){
        char temp = (number % 10) + '0';
        string[digitCounter-i-1] = temp;
        number /= 10;
    }
    string[digitCounter] = '\0';
    return string;
}