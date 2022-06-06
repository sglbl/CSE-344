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
#include "../include/servant.h"
#include "../include/common.h"

static volatile sig_atomic_t didSigIntCome = 0;

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
    doServantJob(dirPath, citiesToHandle, ipv4, portNo);

    return 0;
}

void doServantJob(char *dirPath, char *citiesToHandle, char *ipv4Adress, int portNo){
    struct sockaddr_in socketAdress;
    int socketFd;
    printf("Creating servant\n");
    char buffer[1024] = { 0 };
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        errorAndExit("Socket creation error\n");
    }
    
    socketAdress.sin_family = AF_INET;
    socketAdress.sin_port = htons(portNo);
 
    // Convert IPv4 and IPv6 addresses from text to binary
    if( inet_pton(AF_INET /*IPv4 type*/, ipv4Adress, &socketAdress.sin_addr) == -1) {
        errorAndExit("\nInvalid address/ Address not supported \n");
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
