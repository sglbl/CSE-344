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
    DIR *dir;
    struct dirent *ent;
    char *dirPaths[20];
    int i = 0;
    for(i = 0; i < 20; i++){
        dirPaths[i] = malloc(sizeof(char) * (strlen(dirPath) + 2));
        strcpy(dirPaths[i], dirPath);
        strcat(dirPaths[i], "/");
        // printf("dirPath is %s\n", dirPaths[i]);
        dir = opendir(dirPaths[i]);
        if(dir == NULL){
            errorAndExit("Error opening directory\n");
        }
        SgLinkedList *list = calloc(1, sizeof(SgLinkedList));
        list->next = NULL;
        while((ent = readdir(dir)) != NULL){
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }
            else{
                char *filePath = malloc(sizeof(char) * (strlen(dirPaths[i]) + strlen(ent->d_name) + 1));
                strcpy(filePath, dirPaths[i]);
                strcat(filePath, ent->d_name);
                // Adding file to the linked list
                list = addToLinkedList(list, filePath);
            }
        }
        printLinkedList(list);
        closedir(dir);
    }

    int head = 0, tail = 0;
    cityQueueParser(citiesToHandle, &head, &tail);
    printf("Head is %d and tail is %d\n", head, tail);
    connectToTheServer(ipv4Adress, portNo, head, tail);

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

void connectToTheServer(char *ipv4Adress, int portNo, int head, int tail){
    struct sockaddr_in clientAdress = {0};
    struct sockaddr_in servantAdress = {0};

    socklen_t addresslength;

    printf("(%s) Connecting to the server...\n", timeStamp());
    struct sockaddr_in servantSocketAdress;
    int servantSocketFd;
    if ((servantSocketFd = socket(AF_INET/*ipv4*/, SOCK_STREAM, 0)) == -1) {
        errorAndExit("Socket creation error\n");
    }
    
    servantSocketAdress.sin_family = AF_INET;
    servantSocketAdress.sin_port = htons(portNo);
 
    // Ip to binary
    if( inet_pton(AF_INET /*IPv4 type*/, ipv4Adress, &servantSocketAdress.sin_addr) == -1) {
        errorAndExit("Error! Invalid address.\n");
    }

    if ((bind(servantSocketFd, (struct sockaddr *)&servantSocketAdress, sizeof(servantSocketAdress))) == -1)
        errorAndExit("Bind error");

    if ((listen(servantSocketFd, 256/*256 connections will be queued*/)) == -1)
        errorAndExit("Listen error");

    /*--------------Main loop-------------------------------*/
    // Handle connections while no SIGINT
    while (TRUE){
        addresslength = sizeof(clientAdress);
        // printf("Busy waiting checker\n"); // Not busy waiting because waits until get accepted.
        if( (serverSocket = accept(servantSocketFd, (struct sockaddr *)&servantAdress, &addresslength)) < 0){
            errorAndExit("Accept error\n");
        }
        if (serverSocket > 0){
            // Add request to jobs 
            // pthread_mutex_lock(&csMutex);
            // add_request(serverSocket);
            printf("Accepted\n\n");
            // Sending head and tail information
            if(send(serverSocket, &head, sizeof(int), 0) == -1){
                errorAndExit("Error sending head\n");
            }
            if(send(serverSocket, &tail, sizeof(int), 0) == -1){
                errorAndExit("Error sending tail\n");
            }
            ++portNo;
            if(send(serverSocket, &portNo, sizeof(int), 0) == -1){
                errorAndExit("Error sending tail\n");
            }
            
            // pthread_mutex_unlock(&csMutex);
            // pthread_cond_signal(&cond_job); // Signal new job
        }
    }

    close(serverSocket);


}

SgLinkedList *addToLinkedList(SgLinkedList *head, char *filePath){ //çço1
    SgLinkedList *tempIterator = head;
    while(tempIterator->next != NULL){
		tempIterator = tempIterator->next;	
	}
	tempIterator->next=(SgLinkedList*)calloc(1, sizeof(SgLinkedList));
    // int stringLength = strlen(filePath);
    // tempIterator->string.filePath = calloc(stringLength + 1, sizeof(char));
    // strncpy(tempIterator->string.filePath, filePath, stringLength);
    // tempIterator->next->string.length = stringLength + 1;
    tempIterator->string.data = filePath;

	tempIterator->next->next=NULL;

    return head;
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