#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/stat.h> // Stat command
#include <sys/mman.h> // Memory mapping
#include <semaphore.h> // Semaphore
#include "sync_unnamed.h"

SharedMemory *sharedMemory;
static int lineNumber = 0;
static char (*ingredientsInFile)[2];
static volatile sig_atomic_t signalFlag = 0; // Flag for signal handling

void signalHandlerInitializer(){
    // Initializing siggal action for SIGINT signal.
    struct sigaction actionForSigInt;
    memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    actionForSigInt.sa_handler = sg_signalHandler; // Setting the handler function.
    actionForSigInt.sa_flags = 0; // No flag is set.
    if (sigaction(SIGINT, &actionForSigInt, NULL) < 0){ // Setting the signal.
        errorAndExit("Error while setting SIGINT signal. ");
    }
}

// Create a signal handler function
void sg_signalHandler(int signalNumber){
    if( signalNumber == SIGINT){
        signalFlag = 1;   // writing to static volative. Make signal flag 1 (in order to check later)
    }
}

void arrayStorer(char* inputFilePath){
    signalHandlerInitializer();
    struct stat statOfFile;                 // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;
    if( stat(inputFilePath, &statOfFile) < 0){
        dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
        errorAndExit("Error while opening file to read. ");
    }

    // Opening file in read mode
    if( (fileDesc = open(inputFilePath, O_RDONLY, 0666)) == -1 ){
        dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
        errorAndExit("Error while opening file to read. ");
    }

    // Understanding how many lines are there in input file
    if( statOfFile.st_size % 2 == 0 ){
        statOfFile.st_size = (statOfFile.st_size + 1) / 3 * 2; // Removing new line character from the size of file
    }
    else{
        statOfFile.st_size = statOfFile.st_size / 3 * 2; // Removing new line character from the size of file
    }

    int size = statOfFile.st_size/2;
    // char (*ingredientsInFile)[2];
    ingredientsInFile = calloc(size, sizeof(char)); // Allocating memory for ingredients array
    
    // char ingredients[size][2];
    for(int i = 0; i<size; i++){
        if( read(fileDesc, ingredientsInFile[i], 2) == -1 && errno == EINTR )
            errorAndExit("Error while reading file to array");
        // dprintf(STDOUT_FILENO, "Ingredient %d: %s\n", i, ingredientsInFile[i]);
        char garbage;
        if( read(fileDesc, &garbage, 1) == 0 ) // Don't add garbage '\n' value to file and if all file is readed, then break.
            break;
    }
    close(fileDesc);
    
    // Initialization of shared memory
    int shmId = shm_open("sharedMemory", O_CREAT | O_RDWR, 0666);
    if( shmId == -1 ){
        errorAndExit("Error while opening shared memory ");
    }
    if( ftruncate(shmId, sizeof(ingredientsInFile)) == -1 ){
        errorAndExit("Error while truncating shared memory ");
    }

    void *sharedMemoryPtr = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    sharedMemory = (SharedMemory*) sharedMemoryPtr;
    if( sharedMemory == MAP_FAILED )
        errorAndExit("Error while mapping shared memory ");
    
    // Writing array to shared memory
    sharedMemory->numberOfLines = size;
    sharedMemory->ingredients[0] = ingredientsInFile[0][0];
    sharedMemory->ingredients[1] = ingredientsInFile[0][1];
    sharedMemory->totalNumberOfDesserts = 0;
    lineNumber = 1;
}

void semaphoreInitializer(){
    // Initializing semaphores
    for(int i = 0; i < NUMBER_OF_CHEFS; i++)
        if( sem_init(&(sharedMemory->signalToChef[i]), 1, 0) < 0 )
            errorAndExit("Semaphore initialize error ");

    if( sem_init(&sharedMemory->accessing, 1 /* Pshare */, 1) < 0 )
        errorAndExit("Semaphore initialize error ");
    // if( sem_init(&sharedMemory->wholesaler, 1 /* Pshare */, 1) < 0 )
    //     errorAndExit("Semaphore initialize error ");

    if( sem_init(&sharedMemory->milk, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->flour, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->sugar, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnut, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    
    if( sem_init(&sharedMemory->milkAndFlourSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->flourAndSugarSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->milkAndSugarSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnutAndFlourSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnutAndMilkSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnutAndSugarSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->dessertPrepared, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");

}


void problemHandler(){
    semaphoreInitializer();

    pid_t pidsFromFork[NUMBER_OF_CHEFS + NUMBER_OF_INGREDIENTS];
    pid_t pid;
    for(int i = 0; i < NUMBER_OF_INGREDIENTS; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / PUSHER i *********/
            while(TRUE){
                if     (i == 0) pusherFlour();
                else if(i == 1) pusherMilk();
                else if(i == 2) pusherSugar();
                else if(i == 3) pusherWalnut();
                // return;
            }
        }
        else if( pid > 0 ){
            /********* PARENT *********/
            pidsFromFork[i] = pid;
            dprintf(STDOUT_FILENO, "Pusher id is %d\n", pid);
            int status;
            waitpid(pid, &status, WNOHANG/* Don't wait for child process to finish */);
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process for pushers. ");
        }
    } // End of for loop for creating child processes for pushers

    int i;
    for(i = 0; i < NUMBER_OF_CHEFS; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / CHEF i *********/
            sharedMemory->totalNumberOfDesserts += chef(i); //çç
            // WEXITSTATUS(status);
            return;
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            pidsFromFork[i + NUMBER_OF_INGREDIENTS] = pid;
            int status;
            int waitpidReturnVal = waitpid(0, &status, WNOHANG | WUNTRACED /* Don't wait for child process to finish */);
            if(waitpidReturnVal  < 0)   errorAndExit("Error while using waitpid\n");
            if( WIFEXITED(status) ){
                printf("+-+-+Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
            }
            if(waitpidReturnVal == 0)   dprintf(STDOUT_FILENO, "Child is still runnning\n");
            if(waitpidReturnVal  > 0)   dprintf(STDOUT_FILENO, "||| Child is finished and status is %d\n", status);

            printf("------Status of pid %d is %d\n", pid, status);
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process for chefs. ");
        }
    } // End of for loop for creating child processes for chefs

    // Do wholesaler job.
    if(pid > 0){  // parent
        wholesalerProcess(pidsFromFork);
        sleep(1); //çç
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) is done (Total desserts: %d)\n", getpid(), sharedMemory->totalNumberOfDesserts);
    }

}

void wholesalerProcess(pid_t pidsFromFork[]){
    while(TRUE){
        int foundFLag1 = FALSE, foundFLag2 = FALSE;
        // sem_wait(&sharedMemory->wholesaler);
        switch(sharedMemory->ingredients[0]){
            case 'M':   sem_post(&sharedMemory->milk);   foundFLag1 = TRUE; break;
            case 'W':   sem_post(&sharedMemory->walnut); foundFLag1 = TRUE; break;
            case 'F':   sem_post(&sharedMemory->flour);  foundFLag1 = TRUE; break;
            case 'S':   sem_post(&sharedMemory->sugar);  foundFLag1 = TRUE; break;
        }
        switch(sharedMemory->ingredients[1]){
            case 'M':   sem_post(&sharedMemory->milk);   foundFLag2 = TRUE; break;
            case 'W':   sem_post(&sharedMemory->walnut); foundFLag2 = TRUE; break;
            case 'F':   sem_post(&sharedMemory->flour);  foundFLag2 = TRUE; break;
            case 'S':   sem_post(&sharedMemory->sugar);  foundFLag2 = TRUE; break;
        }

        // If a dessert is prepared, then let program continue with next line.
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) delivers %s and %s.\n", getpid(), 
                                stringConverter(sharedMemory->ingredients[0]), stringConverter(sharedMemory->ingredients[1]) );
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) is waiting for the dessert.\n", getpid());
        sem_wait(&sharedMemory->dessertPrepared);
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) has obtained the dessert and left.\n", getpid());   

        // Changing with new line for the next case.
        if(foundFLag1 == TRUE && foundFLag2 == TRUE){
            sharedMemory->ingredients[0] = ingredientsInFile[lineNumber][0];
            sharedMemory->ingredients[1] = ingredientsInFile[lineNumber][1];
        }
        // dprintf(STDOUT_FILENO, "--Entering line number %d--\n", lineNumber);
        if(lineNumber == sharedMemory->numberOfLines || signalFlag == 1){
            if( signalFlag == 1 ){
                dprintf(STDOUT_FILENO, "\n\n----Signal flag is 1\n");
            }
            // Killing the pusher processes with SIGKILL
            for(int i = 0; i < NUMBER_OF_INGREDIENTS; i++)
                if( kill(pidsFromFork[i], SIGKILL) == -1 )
                    errorAndExit("Killing child failed.\n");
            // (Gracefully) Killing the chef processes with SIGINT
            for(int i = 0; i < NUMBER_OF_CHEFS; i++)
                if( kill(pidsFromFork[i + NUMBER_OF_INGREDIENTS], SIGINT) == -1 )
                    errorAndExit("Killing child failed.\n");
            
            break;
        }
        lineNumber++;
    }
}

int chef(int chefNumber){
    int counter = 0;
    while(TRUE){
        /* 
            chef0 has an endless supply of milk and flour but lacks walnuts and sugar, 
            chef1 has an endless supply of milk and sugar but lacks flour and walnuts, 
            chef2 has an endless supply of milk and walnuts but lacks sugar and flour, 
            chef3 has an endless supply of sugar and walnuts but lacks milk and flour, 
            chef4 has an endless supply of sugar and flour but lacks milk and walnuts, 
            chef5 has an endless supply of flour and walnuts but lacks sugar and milk. 
        */
        switch(chefNumber){
            case 0: // Waiting for WALNUT and SUGAR [has milk and flour]
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) is waiting for walnut and sugar.\n", getpid());
                sem_wait(&sharedMemory->milkAndFlourSignal);
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) has taken the walnut\nChef0 (pid %d) has taken the sugar.\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) has delivered the dessert.\n", getpid());
                break;
            case 1: // Waiting for FLOUR and WALNIT [has milk and sugar]
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) is waiting for milk and sugar.\n", getpid());
                sem_wait(&sharedMemory->milkAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) has taken the walnut\nChef0 (pid %d) has taken the flour.\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) has delivered the dessert.\n", getpid());
                break;
            case 2: // Waiting for SUGAR and FLOUR [has milk and walnuts]
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) is waiting for sugar and flour.\n", getpid());
                sem_wait(&sharedMemory->walnutAndMilkSignal);
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) has taken the flour\nChef2 (pid %d) has taken the sugar\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) has delivered the dessert.\n", getpid());
                break;
            case 3: // Waiting for MILK and FLOUR [has sugar and walnuts]
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) is waiting for milk and flour.\n", getpid());
                sem_wait(&sharedMemory->walnutAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) has taken the flour\nChef3 (pid %d) has taken the milk\n",  getpid(),  getpid());
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) has delivered the dessert.\n", getpid());
                break;
            case 4: // Waiting for  MILK and WALNUTS [has sugar and flour]
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) is waiting for milk and walnuts.\n", getpid());
                sem_wait(&sharedMemory->flourAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) has taken the milk\nChef4 (pid %d) has taken the walnuts\n", getpid(),  getpid());
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) has delivered the dessert.\n", getpid());
                break;
            case 5: // Waiting for SUGAR and MILK [has flour and walnuts]
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) is waiting for sugar and milk.\n", getpid());
                sem_wait(&sharedMemory->walnutAndFlourSignal);
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) has taken the milk\nChef5 (pid %d) has taken the sugar\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) is preparing the dessert.\n", getpid());
                // Dessert is prepared so let program continue with next line of file
                sem_post(&sharedMemory->dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) has delivered the dessert.\n", getpid());
                break;
            default:
                errorAndExit("Chef number is not valid. ");
        }
        counter++;
        if(signalFlag == 1){
            printf("\nChef%d (pid %d) is exiting...\n", chefNumber, getpid());
            dprintf(STDOUT_FILENO, "Chef%d prepared %d desserts.\n", chefNumber, counter);
            return counter;
        }
    }
}

/********************** PUSHERS **********************/
void pusherMilk(){ //Wakes up when there is milk by wholesaler
    sem_wait(&sharedMemory->milk);
    sem_wait(&sharedMemory->accessing);

    if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(&sharedMemory->walnutAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(&sharedMemory->walnutAndFlourSignal);        
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(&sharedMemory->flourAndSugarSignal);
    }
    else
        sharedMemory->isMilk = 1;

    sem_post(&sharedMemory->accessing);
}

void pusherFlour(){
    sem_wait(&sharedMemory->flour);
    sem_wait(&sharedMemory->accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(&sharedMemory->walnutAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(&sharedMemory->walnutAndMilkSignal);
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(&sharedMemory->milkAndSugarSignal);
    }
    else
        sharedMemory->isFlour = 1;

    sem_post(&sharedMemory->accessing);
}

void pusherSugar(){
    sem_wait(&sharedMemory->sugar);
    sem_wait(&sharedMemory->accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(&sharedMemory->walnutAndFlourSignal);
    }
    else if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(&sharedMemory->walnutAndMilkSignal);
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(&sharedMemory->milkAndFlourSignal);
    }
    else
        sharedMemory->isSugar = 1;

    sem_post(&sharedMemory->accessing);
}

void pusherWalnut(){
    sem_wait(&sharedMemory->walnut);
    sem_wait(&sharedMemory->accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(&sharedMemory->flourAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(&sharedMemory->milkAndFlourSignal);
    }
    else if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(&sharedMemory->milkAndSugarSignal);
    }
    else
        sharedMemory->isWalnut = 1;

    sem_post(&sharedMemory->accessing);

}

void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

char *stringConverter(char character){
    if(character == 'W') return "walnuts";
    if(character == 'S') return "sugar";
    if(character == 'M') return "milk";
    if(character == 'F') return "flour";
    else return "error";
}