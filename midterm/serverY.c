#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/file.h> // For flock (to apply/remove an consultative lock on an open file)
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include <semaphore.h> // Semaphore
#include <sys/shm.h>
#include <sys/mman.h>
#include "serverY.h"
#include "prerequisites.h"

static sem_t *sem;
static int logFileDesc_s, serverFileDesc, dummyFileDesc, closerFileDesc, pidFileDesc_s;
static char *serverFifoPath;
static volatile sig_atomic_t didSigIntCome = 0;

void whileExiting(){
    unlink(serverFifoPath);
    if( close(serverFileDesc) < 0)  sg_perrorAndExit("Error while closing server fifo descriptor ", logFileDesc_s, 0);
    if( close(closerFileDesc) < 0)  sg_perrorAndExit("Error while closing null dev file descriptor ", logFileDesc_s, 0);
    if( close(pidFileDesc_s) < 0)  sg_perrorAndExit("Error while closing pid file descriptor ", logFileDesc_s, 0);

    
    sem_close(sem); close(dummyFileDesc);
    sem_unlink("/semaphore1");
    char *args[] = {"/bin/kill", "-9 $(ps -C serverY -o pid=)", NULL};  // search daemon of serverY and get pid to kill it)
    switch (fork()){
        case -1:     // Error
            exit(EXIT_FAILURE);
        case 0:      // Child
            execv("/bin/kill", args); //Will kill daemon(itself)
            perror("execv error"); 
            exit(EXIT_FAILURE);
        default:    //Parent
            wait(NULL); // To kill zombie process wait until child terminates.
    }
    
    dprintf(logFileDesc_s,"SIGINT received, terminating Z and exiting server Y. ");
    if( close(logFileDesc_s) < 0)  sg_perrorAndExit("Error while closing log file descriptor ", STDOUT_FILENO, 0);
    
}

int main(int argc, char *argv[]){
    // Initialization of blocking signals
    sigset_t blockSetMask, oldSetMask;  // Set of signals to block
    sigemptyset(&blockSetMask); // Initializing the set of signals to block.
    sigemptyset(&oldSetMask);   // To set old mask at the end.
    sigfillset(&blockSetMask);  // Filling set will all signals. [SIGKILL and SIGSTOP cannot be blocked]
    sigdelset(&blockSetMask, SIGINT); // Remove SIGINT from the set of signals to block [because we want to use the signal handler function for SIGINT]
    // Blocking signals
    if( sigprocmask(SIG_BLOCK, &blockSetMask, &oldSetMask) == -1 ){
        sg_perrorAndExit("sigprocmask error ", logFileDesc_s, 0);
        exit(EXIT_FAILURE);
    }

    int poolSizeY = 0, poolSizeZ = 0, sleepTime = 0, option;     // Argument options
    char *logFilePath;

    while ((option = getopt(argc, argv, "s:o:p:r:t:")) != -1) {
        switch (option) {
            case 's':
                serverFifoPath = optarg;
                break;
            case 'o':
                logFilePath = optarg;
                break;
            case 'p':
                poolSizeY = atoi(optarg);
                break;
            case 'r':
                poolSizeZ = atoi(optarg);
                break;
            case 't':
                sleepTime = atoi(optarg);
                break;
            default:
                write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
                exit(EXIT_FAILURE);
        }
    }

    handleClientRequest(logFilePath, poolSizeY, poolSizeZ, sleepTime);

    // Unblocking signals
    if( sigprocmask(SIG_UNBLOCK, &oldSetMask, NULL) == -1 ){
        sg_perrorAndExit("sigprocmask error ", logFileDesc_s, 0);
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Successfully exiting\n", 21);
    exit(EXIT_SUCCESS);
}

void handleClientRequest(char *logFilePath, int poolSizeY, int poolSizeZ, int sleepTime){
    int logFileDesc;
    int readedByte;
    int sequenceNumber = 0;
    
    signalHandlerInitializer();
    
    // If the first instance, returns FALSE.
    if( singletonMakerAndCheckIfRunningAlready() == TRUE ){
        write(STDERR_FILENO, "Double instantiation isn't allowed. Remove serverFifo.txt in order to continue\n", 36);
        exit(EXIT_SUCCESS);
    }

    // Also singleton checker. Creating server fifo
    if( mkfifo(serverFifoPath, 0666) < 0 ){
        sg_perrorAndExit("Error while creating server fifo. Double instantiation isn't allowed ", STDERR_FILENO, 0);
    }

    if( daemonMaker() == -1 ){
        sg_perrorAndExit("Error while trying to demonize", STDERR_FILENO, 0);
    }

    if( (logFileDesc = open(logFilePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1){
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 54);
        sg_perrorAndExit("Error while opening log file. ", STDERR_FILENO, 0);
    }
    logFileDesc_s = logFileDesc;
    if( atexit(whileExiting) == -1 )
        sg_perrorAndExit("Error while exiting", logFileDesc, 0);

    dprintf(logFileDesc, "(%s) Server Y (p=%d, t=%d) started\n", timeStamp(), poolSizeY, sleepTime);
    // Instantiating server Z / Creating serverZ with fork
    int yzPipeFileDesc[2];
    if(pipe(yzPipeFileDesc) < 0)
        sg_perrorAndExit("Error while creating pipe", logFileDesc, 0);
        
    pid_t fork1 = fork();
    if( fork1 == 0 ){
        // Child serverZ
        if( close(yzPipeFileDesc[1]) < 0 )  // closing write end of child
            sg_perrorAndExit("Error while closing read end of pipe", logFileDesc, 0);
        serverZ(logFileDesc, yzPipeFileDesc, poolSizeZ, sleepTime);
        exit(EXIT_SUCCESS);
    }
    else if( fork1 < 0 ){
        sg_perrorAndExit("Error while forking", logFileDesc, 0);
    }
    else{
        // Parent process
        if( close(yzPipeFileDesc[0]) < 0 )  // closing read end of parent y
            sg_perrorAndExit("Error while closing write end of pipe", logFileDesc, 0);
    }
    dprintf(logFileDesc, "(%s) Instantiated server Z\n", timeStamp());

    // Opening server fifo in read mode
    if( (serverFileDesc = open(serverFifoPath, O_RDONLY, S_IWGRP)) < 0 ){
        sg_perrorAndExit("Error while opening server fifo to write. :", logFileDesc, 0);
    }

    dummyFileDesc = open(serverFifoPath, O_WRONLY);
    if( dummyFileDesc < 0){
        sg_perrorAndExit("Dummy file descriptor error:", logFileDesc, 0);
    }

    int (*fileDesc)[2] = calloc(poolSizeY, sizeof(int));

    pid_t forkVal;
    int i;

    int shm_fd;
    void* ptr;
    char * name = "availabilityOfChildOfY";
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, poolSizeY);
    ptr = mmap(0, poolSizeY, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    int * available = (int*) ptr; // i=0 not available, 1 available

    for(int i=0; i < poolSizeY; i++ ){
        available[i]  = 1;
    }

    for(i=0; i < poolSizeY; i++){
        if(pipe(fileDesc[i]) < 0)
            sg_perrorAndExit("Error while creating pipe", logFileDesc, 0);
        
        forkVal = fork();
        
        if(forkVal == -1){
            sg_perrorAndExit("Error while forking : ", logFileDesc, 0);
        }
        if(forkVal == 0){ // child
            break;
        }
        else{
            // Parent
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
        }
    }


    if(forkVal == 0){ //child
        if( close(fileDesc[i][1]) < 0 )  // closing write end of child
            sg_perrorAndExit("Error while closing write end", logFileDesc, 0);
        while(TRUE){
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Z.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            //******************** CHILD ********************//
            ServerResponse response;
            ClientRequest tempReq;
            ClientRequest *requests = calloc(1, sizeof(ClientRequest));
            response.serverPid = getpid();
            
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            
            if( (readedByte = read(fileDesc[i][0], &tempReq, sizeof(tempReq)) ) != sizeof(tempReq) && errno == EINTR ){
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "SIGINT signal received, exiting server Y.\n");
                    exit(EXIT_SUCCESS);
                }
                sg_perrorAndExit("Error for request of reading so passing : ", logFileDesc, NO_EXIT);
                continue;
            }
            available[i] = 0;
            size_t reqSize = sizeof(*requests) + tempReq.totalMatrixSize * sizeof(requests->matrix[0]); // Using FAC from C99
            if((requests = malloc( reqSize )) == NULL){
                sg_perrorAndExit("Error while allocating memory for requests : ", logFileDesc, 0);
            }
            requests = &tempReq;
            int bytesToRead = tempReq.totalMatrixSize * sizeof(requests->matrix[0]);
            if( read(fileDesc[i][0], requests->matrix, bytesToRead) != bytesToRead ){
                sg_perrorAndExit("Error while reading matrix from server fifo : ", logFileDesc, 0);
            }
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            
            // Opening client's fifo to write the response to it.
            char *clientFifo = calloc( CLIENT_FIFO_PATH_NAME_SIZE, sizeof(char));
            int clientFileDesc;
            strcpy( clientFifo, TEMP_CLIENT_FIFO );
            char *requestPid = itoaForAscii(requests->pid) ;
            strcat( clientFifo, requestPid);
        
            dprintf(logFileDesc, "(%s) Worker PID#%d is handling client PID#%d, matrix size %dx%d, pool busy %d/%d\n",
                    timeStamp(), getpid(), requests->pid, requests->rowSize, requests->rowSize, i+1, poolSizeY);
            

            if( (clientFileDesc = open(clientFifo, O_WRONLY)) < 0 ){
                sg_perrorAndExit("Error while opening client fifo to write. ", logFileDesc, NO_EXIT);
                continue; // Give up on client
            }
            free(clientFifo);
            response.serverPid = getpid();
            response.sequenceNumber = sequenceNumber;
            response.isInvertible = checkIfMatrixIsInvertible(requests->matrix, requests->rowSize, logFileDesc);
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            sleep(sleepTime);
            
            if(response.isInvertible == FALSE)
                dprintf(logFileDesc, "(%s) Worker PID#%d responding to client PID#%d: the matrix IS NOT invertible.\n",
                    timeStamp(), getpid(), requests->pid);
            else
                dprintf(logFileDesc, "(%s) Worker PID#%d responding to client PID#%d: the matrix IS invertible.\n",
                    timeStamp(), getpid(), requests->pid);

            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            
            // Writing the response to client's fifo
            if( (write(clientFileDesc, &response, sizeof(ServerResponse))) != sizeof(ServerResponse) ){
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                timeStampPrinter(logFileDesc);
                dprintf(logFileDesc,"Error while writing to client fifo. \n");
                continue; // Give up on client
            }   
            
            available[i] = 1;
            if(close(clientFileDesc) < 0){
                sg_perrorAndExit("Error while closing client fifo. ", logFileDesc, 0);
                exit(EXIT_FAILURE);
            }

            sequenceNumber += requests->sequenceLength;
            
            // _exit(EXIT_SUCCESS);
        }
    }
    else{
        //*******************  PARENT ***************************/
        ClientRequest tempReq;
        ClientRequest *requests = calloc(1, sizeof(ClientRequest));
        for(int j=0; j<poolSizeY; j++)
            if( close(fileDesc[j][0]) < 0 )  // closing read end of parent
                sg_perrorAndExit("Error while closing read end of parent", logFileDesc, 0);
        while(TRUE){
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received. Exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            
            // int waitVal = waitpid(childPids[i], NULL, WNOHANG); // return status for stopped children
            // if(waitVal == -1 && errno != ECHILD){
            //     sg_perrorAndExit("Error on waitpid() command ", logFileDesc, 0);
            // }
            // else if (waitVal != 0) {
            //     /* Child is done */
            //     childPids[i] = 0; // flag it as done
            // }
            // else { // Child is still running (not already dead)
                /* Still waiting on this child */
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                
                if( (readedByte = read(serverFileDesc, &tempReq, sizeof(tempReq))) != sizeof(tempReq) && errno == EINTR ){
                    if(didSigIntCome == 1){
                        timeStampPrinter(logFileDesc);
                        dprintf(logFileDesc, "SIGINT signal received. Terminating Z and exiting server Y.\n");
                        exit(EXIT_SUCCESS);
                    }
                    sg_perrorAndExit("Error for request of reading so passing : ", logFileDesc, NO_EXIT);
                    continue;
                }
                
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) SIGINT signal received. Terminating Z and exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                size_t reqSize = sizeof(*requests) + tempReq.totalMatrixSize * sizeof(requests->matrix[0]); // Using FAC from C99
                if((requests = malloc( reqSize )) == NULL){
                    sg_perrorAndExit("Error while allocating memory for requests : ", logFileDesc, 0);
                }
                requests = &tempReq;
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                int bytesToRead = tempReq.totalMatrixSize * sizeof(requests->matrix[0]);
                if( read(serverFileDesc, requests->matrix, bytesToRead) != bytesToRead ){
                    if(didSigIntCome == 1){
                        dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                        exit(EXIT_SUCCESS);
                    }
                    sg_perrorAndExit("Error while reading all of matrix from server fifo : ", logFileDesc, 0);
                }

                // writing readed things from client to the pipe
                int requestSize = sizeof(tempReq) + tempReq.totalMatrixSize*sizeof(tempReq.matrix[0]);
                for(int j=0; j<poolSizeY; j++){
                    if(available[j] == 1){ //available[j] == 1 means that the worker is not busy
                        if( write(fileDesc[j][1], &tempReq, requestSize) == -1 ){
                            if(didSigIntCome == 1){
                                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                                exit(EXIT_SUCCESS);
                            }
                            sg_perrorAndExit("Error while sending request information to child. ", logFileDesc, NO_EXIT);
                            write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
                            exit(EXIT_FAILURE);
                        }
                        if(didSigIntCome == 1){
                            dprintf(logFileDesc, "(%s) SIGINT signal received. Terminating Z and exiting server Y.\n", timeStamp());
                            exit(EXIT_SUCCESS);
                        }
                        break;
                    }
                    if(j == poolSizeY - 1){ // Using Server-Z
                        // Writing to the yzPipeFileDesc (server-z)
                        if( (write(yzPipeFileDesc[1], &tempReq, requestSize)) != requestSize ){
                            if(didSigIntCome == 1){
                                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                                exit(EXIT_SUCCESS);
                            }
                            sg_perrorAndExit("Error while writing to server-z fifo. ", logFileDesc, 0);
                        }
                        break;
                    }
                    
                }
            // }
        
            /* Preventing hard loop with giving up time slice */
            // sleep(0);
        }
        free(requests);
        // free(childPids);
        free(fileDesc);
    }
}

void serverZ(int logFileDesc, int yzPipeFileDesc[/*2*/], int poolSizeZ, int sleepTime){
    // Initialization of blocking signals
    sigset_t blockSetMask, oldSetMask;  // Set of signals to block
    sigemptyset(&blockSetMask); // Initializing the set of signals to block.
    sigemptyset(&oldSetMask);   // To set old mask at the end.
    sigfillset(&blockSetMask);  // Filling set will all signals. [SIGKILL and SIGSTOP cannot be blocked]
    sigdelset(&blockSetMask, SIGINT); // Remove SIGINT from the set of signals to block [because we want to use the signal handler function for SIGINT]
    // Blocking signals
    if( sigprocmask(SIG_BLOCK, &blockSetMask, &oldSetMask) == -1 ){
        sg_perrorAndExit("sigprocmask error ", logFileDesc, 0);
        exit(EXIT_FAILURE);
    }

    sem = sem_open("/semaphore1", O_CREAT,  0644, 0);

    // Initialization of shared memory
    char *name = "availabilityOfChildOfZ";
    int shmAvailable = shm_open(name, O_CREAT | O_RDWR, 0666);
    if( shmAvailable == -1 ){
        sg_perrorAndExit("Error while opening shared memory ", logFileDesc, 0);
    }
    if( ftruncate(shmAvailable, sizeof(SharedMemoryForZ)) == -1 ){
        sg_perrorAndExit("Error while truncating shared memory ", logFileDesc, 0);
    }

    void* ptr = mmap(0, poolSizeZ, PROT_WRITE, MAP_SHARED, shmAvailable, 0);
    int * available = (int*) ptr; // i=0 not available, 1 available

    for(int i=0; i < poolSizeZ; i++ ){
        available[i]  = 1;
    }

    // Initialization of shared memory
    int shmId = shm_open("sharedMemoryForZ", O_CREAT | O_RDWR, 0666);
    if( shmId == -1 ){
        sg_perrorAndExit("Error while opening shared memory ", logFileDesc, 0);
    }
    if( ftruncate(shmId, sizeof(SharedMemoryForZ)) == -1 ){
        sg_perrorAndExit("Error while truncating shared memory ", logFileDesc, 0);
    }

    void *sharedMemoryPtr = mmap(NULL, sizeof(SharedMemoryForZ), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    SharedMemoryForZ *sharedMemory = (SharedMemoryForZ*) sharedMemoryPtr;
    if( sharedMemory == MAP_FAILED ){
        sg_perrorAndExit("Error while mapping shared memory ", logFileDesc, 0);
    }
    
    // Reading from pipe and writing to log file
    int readedByte;
    if(didSigIntCome == 1){
        dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
        exit(EXIT_SUCCESS);
    }

    int i, forkVal;
    for(i=0; i < poolSizeZ; i++){
        // Instead pipe use shared memory
        forkVal = fork();
        if(forkVal == -1){
            sg_perrorAndExit("Error while forking : ", logFileDesc, 0);
        }
        if(forkVal == 0){ // child
            break;
        }
        else{
            // Parent
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Z.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
        }
    }

    if(forkVal == 0){  // child
        //**************** SERVER Z CHILD ********************//
        while(TRUE){
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Z.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            sem_wait(sem);
            if(sharedMemory->isThereRequest == 1){
                ServerResponse response;
                ClientRequest *requests = calloc(1, sizeof(ClientRequest));
                requests = &sharedMemory->clientRequest;
                
                // Opening client's fifo to write the response to it.
                char *clientFifo = calloc( CLIENT_FIFO_PATH_NAME_SIZE, sizeof(char));
                int clientFileDesc;
                strcpy( clientFifo, TEMP_CLIENT_FIFO );
                char *requestPid = itoaForAscii( requests->pid );
                strcat( clientFifo, requestPid);
                // free(requestPid);
                dprintf(logFileDesc, "(%s) Z: Worker PID#%d is handling client PID#%d, matrix size %dx%d, pool busy %d/%d\n",
                        timeStamp(), getpid(), requests->pid, requests->rowSize, requests->rowSize, i+1, poolSizeZ);
                
                if( (clientFileDesc = open(clientFifo, O_WRONLY)) < 0 ){
                    sg_perrorAndExit("Z: Error while opening client fifo to write. ", logFileDesc, 0);
                }
                free(clientFifo);
                response.serverPid = getpid();
                response.isInvertible = checkIfMatrixIsInvertible(requests->matrix, requests->rowSize, logFileDesc);
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) Z: SIGINT signal received, exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                sleep(sleepTime);
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Z.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                if(response.isInvertible == FALSE)
                    dprintf(logFileDesc, "(%s) Z: Worker PID#%d responding to client PID#%d: the matrix IS NOT invertible.\n",
                        timeStamp(), getpid(), requests->pid);
                else
                    dprintf(logFileDesc, "(%s) Z: Worker PID#%d responding to client PID#%d: the matrix IS invertible.\n",
                        timeStamp(), getpid(), requests->pid);

                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "(%s) Z: SIGINT signal received, exiting server Y.\n", timeStamp());
                    exit(EXIT_SUCCESS);
                }
                
                // Writing the response to client's fifo
                if( (write(clientFileDesc, &response, sizeof(ServerResponse))) != sizeof(ServerResponse) ){
                    if(didSigIntCome == 1){
                        dprintf(logFileDesc, "(%s) Z: SIGINT signal received, exiting server Y.\n", timeStamp());
                        exit(EXIT_SUCCESS);
                    }
                    timeStampPrinter(logFileDesc);
                    dprintf(logFileDesc,"Z: Error while writing to client fifo. \n");
                    exit(EXIT_FAILURE);
                }   
                
                available[i] = 1;
                if(close(clientFileDesc) < 0){
                    sg_perrorAndExit("Z: Error while closing client fifo. ", logFileDesc, 0);
                    exit(EXIT_FAILURE);
                }
                sharedMemory->isThereRequest = 0;
                
            }
        }

    }else{
        //**************** SERVER Z PARENT ********************//
        ClientRequest tempReq;
        ClientRequest *requests = calloc(1, sizeof(ClientRequest));
        
        while(TRUE){
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Z.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            if( (readedByte = read(yzPipeFileDesc[0], &tempReq, sizeof(tempReq)) ) != sizeof(tempReq) && errno == EINTR ){
                if(didSigIntCome == 1){
                    dprintf(logFileDesc, "SIGINT signal received, exiting server Y.\n");
                    exit(EXIT_SUCCESS);
                }
                sg_perrorAndExit("Error for request of reading so passing : ", logFileDesc, NO_EXIT);
                continue;
            }
            available[i] = 0;
            size_t reqSize = sizeof(*requests) + tempReq.totalMatrixSize * sizeof(requests->matrix[0]); // Using FAC from C99
            if((requests = malloc( reqSize )) == NULL){
                sg_perrorAndExit("Error while allocating memory for requests : ", logFileDesc, 0);
            }
            requests = &tempReq;
            int bytesToRead = tempReq.totalMatrixSize * sizeof(requests->matrix[0]);
            if( read(yzPipeFileDesc[0], requests->matrix, bytesToRead) != bytesToRead ){
                sg_perrorAndExit("Error while reading matrix from server fifo : ", logFileDesc, 0);
            }
            if(didSigIntCome == 1){
                dprintf(logFileDesc, "(%s) SIGINT signal received, exiting server Y.\n", timeStamp());
                exit(EXIT_SUCCESS);
            }
            
            sharedMemory->poolSizeZ = poolSizeZ;
            sharedMemory->sleepTime = sleepTime;
            sharedMemory->clientRequest = *requests;
            dprintf(logFileDesc, "(%s) Z: Worker PID#%d received request from client PID#%d, matrix size %dx%d, pool busy %d/%d\n",
                    timeStamp(), getpid(), requests->pid, requests->rowSize, requests->rowSize, i+1, poolSizeZ);
            sharedMemory->isRunning = 1;
            sharedMemory->isThereRequest = 1;
            sem_post(sem);

        }        

    }

    sem_close(sem);
    sem_unlink("/semaphore1");

}

char *timeStamp(){
    time_t currentTime;
    currentTime=time(NULL);
    char *timeString = asctime( localtime(&currentTime) );

    // Removing new line character from the string
    int length = strlen(timeString);
    if (length > 0 && timeString[length-1] == '\n') 
        timeString[length - 1] = '\0';
    return timeString;
}

void timeStampPrinter(int fileDesc){
    dprintf(fileDesc, "(%s) ", timeStamp());
}

int daemonMaker(){
    int maximumFileDesc;
    // int closerFileDesc;

    int forkVal = fork();     // Making itself as background process
    if(forkVal == -1){  // Child forking error return
        sg_perrorAndExit("Fork error ", logFileDesc_s, 0);
        return -1;  
    }
    if(forkVal ==  0) 
        {/* Do nothing for now */}
    else        // It's parent process so daemon forks and parent dies.
        _exit(EXIT_SUCCESS);

    // (We are in child process) Making itself as new leader of process group by creating a new session without controlling terminal.
    if(setsid() < 0){  // Purpose is free itself of any association with controlling terminal
        sg_perrorAndExit("setsid error\n", logFileDesc_s, 0);
        return -1;
    }
    
    // Ignoring the signal which sent from child to parent process (to prevent zombie process.)
    struct sigaction sigAct;
    sigAct.sa_handler = SIG_IGN; // Ignore SIGCHLD
    if ( sigaction(SIGCHLD, &sigAct, NULL) == -1 ){
        sg_perrorAndExit("Sigaction error ", logFileDesc_s, 0);
        return -1;
    }

    // The double-fork method ensures that the we/daemon process is not the session leader, 
    // for example will guarantee that a call to open isn't gonna be resulted in the daemon process reacquiring a controlling terminal.
    forkVal = fork();
    if(forkVal == -1)  // forking error return
         return -1;  
    if(forkVal ==  0)  // Child is no more process leader and is detached from terminal
        {/* Do nothing */}
    else {
        _exit(EXIT_SUCCESS); // Killing the parent process
    }

    // files or directories which created newly have requested permissions initially
    if( umask(0) < 0)
    // for working on root directory
    chdir("/"); 
    // Closing all file descriptors
    if( (maximumFileDesc = sysconf(_SC_OPEN_MAX)) == -1)
        maximumFileDesc = 8345; // A random big number for maximum file descriptor
    for(closerFileDesc = 0; closerFileDesc < maximumFileDesc; closerFileDesc++)
        close(closerFileDesc);
    
    // set opening STDIN, STDERR and STDOUT to dev/null
    close(STDIN_FILENO);
    closerFileDesc = open("/dev/null", O_RDWR);  // Setting descriptors to /dev/null
    if( closerFileDesc !=  STDIN_FILENO )
        return -1;
    
    if( ( dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO ) || // with dup2 make all those descriptors refer to this device (to /dev/null)
        ( dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO ) // with dup2 make all those descriptors refer to this device (to /dev/null)
         // File descriptor has to be 0
    ){
        return -1;
    }

    return 0;
}

void signalHandlerInitializer(){
    // Initializing signal action for SIGINT signal.
    // struct sigaction actionForSigInt;
    // memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    // actionForSigInt.sa_handler = sg_signalHandler; // Setting the handler function.
    // actionForSigInt.sa_flags = 0; // No flag is set.
    // if (sigaction(SIGINT, &actionForSigInt, NULL) < 0){ // Setting the signal.
    //     sg_perrorAndExit("Error while setting SIGINT signal. ", logFileDesc_s, 0);
    //     exit(EXIT_FAILURE);
    // }
    signal(SIGINT, sg_signalHandler);
}

// Create a signal handler function
void sg_signalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        didSigIntCome = 1;   // writing to static volative. If zero make it 1.
}

int checkIfMatrixIsInvertible(int *matrix, int rowSize, int logFileDesc){
    // If matrix is invertible, then determinant should be different from 0
    long int determinant = determinantFinder(matrix, rowSize);
    // dprintf(logFileDesc, "(%s) Determinant is %ld\n", timeStamp(), determinant);
    if(determinant == 0)
        return FALSE;
    return TRUE;
}

long int determinantFinder(int *matrix, int rowSize){ 
    // Finding determinant with cofactor expansion
    long int determinant = 0;
    // If rowSize == 2, we don't need to use cofactor expansion
    if( rowSize == 2 ){
        determinant = matrix[0]*matrix[3] - matrix[1]*matrix[2];
        return determinant;
    }

    // Using first row to find the determinant
    for(int j = 0; j < rowSize; j++)
        determinant += matrix[0*rowSize + j] * cofactorFinder(matrix, 0, j, /* First row[index = 0], jTh Column */ rowSize);
    return determinant;
}

long int cofactorFinder(int *matrix, int row, int column, int rowSize){
    // Finding Cofactor_row,column
    
    // Cofactor = (-1)^(row+column) * remainingDeterminantWithoutThatRowColumn
    // Cofactor = power * minor
    int power;
    if( (row+column) % 2 == 0 ){
        power = 1; // Because (-1)^(row+column) is positive.
    }else{
        power = -1;
    }

    // Finding minor
    int minorMatrix[(rowSize-1)*(rowSize-1)];
    int counter = 0;
    for(int i = 0; i < rowSize; i++)
        for(int j = 0; j < rowSize; j++)
            if( i != row && j != column ){
                minorMatrix[counter] = matrix[i*rowSize+j];
                counter++;
            }

    return power * determinantFinder(minorMatrix, rowSize-1);  
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

void sg_perrorAndExit(char *errorMessage, int logFileDesc, int noExit){
    dprintf(logFileDesc, "(%s) %s %s\n", timeStamp(), errorMessage, strerror(errno));
    if(noExit != TRUE)
        exit(EXIT_FAILURE);
}

int singletonMakerAndCheckIfRunningAlready(){
    struct flock fileLock;
    memset(&fileLock, 0, sizeof(fileLock)); // Initializing lock

    // Setting the lock for writing and moving cursor to 
    fileLock.l_type = F_WRLCK;
    fileLock.l_whence = SEEK_SET; // Offset from beginning of file
    fileLock.l_start = 0; // Offset where lock begins
    fileLock.l_len = 0; // 0 means Lock entire file
    
    int pidFileDesc;
    // Creating / Opening the file
    if( (pidFileDesc = open(TEMP_PID, O_CREAT | O_RDWR, S_IRUSR|S_IWUSR)) == -1){
        sg_perrorAndExit("Error while opening the file. ", STDOUT_FILENO, NO_EXIT);
    }
    pidFileDesc_s = pidFileDesc;
    // Checking if file is opened and setting the lock
    if( fcntl(pidFileDesc, F_SETLK, &fileLock) < 0 ){ //putting write lock
        // If already running, just return TRUE.
        if(errno == EACCES || errno == EAGAIN){
            close(pidFileDesc);
            return TRUE;
        }else{
            sg_perrorAndExit("Couldn't lock file because of singleton\n", logFileDesc_s, NO_EXIT);
            return TRUE;
        }
    }

    // If file contains something, remove content.
    if( ftruncate(pidFileDesc, 0) < 0){
        sg_perrorAndExit("Error while truncating file\n", logFileDesc_s, NO_EXIT);
    }
    // Convert int to str and write to singleton pid file
    char *pidStr = itoaForAscii(getpid());
    if ( write(pidFileDesc, pidStr, strlen(pidStr)) != strlen(pidStr) ){
        timeStampPrinter(logFileDesc_s);
        sg_perrorAndExit("Error while writing to pidstr file\n", logFileDesc_s, 0);
        exit(EXIT_FAILURE);
    }
    free(pidStr);

    // First instance so, not already running so return FALSE
    return FALSE; 
}