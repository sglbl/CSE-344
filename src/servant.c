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

    // Reading province names from the dataset folder
    struct dirent **allTheEntities;
    int readed;
    if((readed = scandir(dirPath, &allTheEntities, NULL, alphasort)) < 0)
        errorAndExit("scandir error while searching for directories");
    // printf("Found %d directories\n", readed);
    for(int i = 0; i < readed; ++i){
        if(i - 1 >= head && i - 1 <= tail){
            cityList = addCityToLinkedList(cityList, allTheEntities[i]->d_name);
            // printf("Added %s to list\n", allTheEntities[i]->d_name);
        }
        free(allTheEntities[i]);
    }
    free(allTheEntities);


    // Reading inside of the province folders
    DIR *dir;
    struct dirent *ent;
    SgCityLinkedList *cityIter = cityList;

    while(cityIter != NULL && cityIter->cityName.data != NULL){
        cityIter->transactions = calloc(1, sizeof(SgLinkedList));
        cityIter->transactions->next = NULL;
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
                cityIter->transactions = addTransactionToLinkedList(cityIter->transactions, ent->d_name);
            }
        }
        closedir(dir);
        cityIter = cityIter->next;
    }

    // dir = opendir(dirPath[i]);
    // if(dir == NULL){
    //     errorAndExit("Error opening directory\n");
    // }
    // SgLinkedList *list = calloc(1, sizeof(SgLinkedList));
    // list->next = NULL;
    // while((ent = readdir(dir)) != NULL){
    //     // If finds current folder or parent folder, skip it
        
    //     if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
    //         continue;
    //     }
    //     else{
    //         char *filePath = malloc(sizeof(char) * (strlen(dirPaths[i]) + strlen(ent->d_name) + 1));
    //         strcpy(filePath, dirPaths[i]);
    //         strcat(filePath, ent->d_name);
    //         // Adding file to the linked list
    //         list = addToLinkedList(list, filePath);
    //     }
    // }
    // closedir(dir);

    printCityLinkedList(cityList);
    printf("Head is %d and tail is %d\n", head, tail);
    servantTcpCommWithServer(ipv4Adress, portNo, head, tail);
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

void servantTcpCommWithServer(char *ipv4Adress, int portNo, int head, int tail){
    // Create socket
    int serverSocketFd, servantSocketFd;
    if((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errorAndExit("Error creating socket");
    }

    // Connecting to socket of servant
    struct sockaddr_in serverSocketAdressInfo;
    serverSocketAdressInfo.sin_family = AF_INET;
    serverSocketAdressInfo.sin_port = htons(portNo);
    // serverSocketAdressInfo.sin_addr.s_addr = INADDR_ANY;

    if( (servantSocketFd = connect(serverSocketFd, (struct sockaddr *)&serverSocketAdressInfo, sizeof(serverSocketAdressInfo))) < 0)
        errorAndExit("Error connecting to socket");
    if (inet_pton(AF_INET, ipv4Adress, &serverSocketAdressInfo.sin_addr) <= 0) 
        errorAndExit("Inet pton error with ip adress.\n");

    while (TRUE){
        int iAmServant = SERVANT;
        if(send(serverSocketFd, &iAmServant, sizeof(int), 0) == -1){
            errorAndExit("Error sending client info\n");
        }
        int dataSize = 5;
        if(send(serverSocketFd, &dataSize, sizeof(int), 0) == -1){
            errorAndExit("Error sending data size\n");
        }
        // if(send(serverSocketFd, data, dataSize, 0) == -1){
        //     errorAndExit("Error sending data\n");
        // }
        pthread_mutex_unlock(&csMutex);
        int response;
        if(recv(serverSocketFd, &response, sizeof(int), 0) < 0){
            errorAndExit("Error receiving head information");
        }
    }

    close(serverSocket);
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
    // tempIterator->string.data = filePath;
    // printf("Filepath is %s\n", filePath);

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
    // tempIterator->string.data = filePath;
    // printf("Filepath is %s\n", filePath);

	tempIterator->next->next=NULL;

    return head;
}

void printCityLinkedList(SgCityLinkedList *iter){
    printf("(%s) Printing linkedlist values\n", timeStamp());
    while(iter->next != NULL){
        printf("(%s) %s\n", timeStamp(), iter->cityName.data);
        while(iter->transactions->next != NULL){
            printf("(%s) %s\n", timeStamp(), iter->transactions->string.data);
            iter->transactions = iter->transactions->next;
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