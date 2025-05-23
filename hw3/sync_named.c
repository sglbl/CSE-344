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
#include "sync_named.h"

static SharedMemory *sharedMemory;
static sem_t *signalToChef[NUMBER_OF_CHEFS];
static sem_t *accessing, *milk, *flour, *walnut, *sugar;
static sem_t *walnutAndFlourSignal, *walnutAndSugarSignal, *walnutAndMilkSignal, *milkAndSugarSignal, *milkAndFlourSignal, *flourAndSugarSignal;
static sem_t *dessertPrepared, *childReturned;
static int lineNumber = 0;
static char (*ingredientsInFile)[2];
static volatile sig_atomic_t signalFlag = 0; // Flag for signal handling
static char *names[NUMBER_OF_SEMAPHORES];

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

void arrayStorer(char* inputFilePath, char *name){
    signalHandlerInitializer();
    struct stat statOfFile;                 // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;
    if( stat(inputFilePath, &statOfFile) < 0){
        dprintf(STDOUT_FILENO,"Error while opening file to read. ");
        dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
        exit(EXIT_FAILURE);
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
    lineNumber = 1;

    // Saving semaphore names to array with char *name
    for(int i = 0; i < NUMBER_OF_SEMAPHORES; i++){
        names[i] = calloc(strlen(name) + 2, sizeof(char));
        strcpy(names[i], name);
        char *temp = sg_itoa(i);
        strcat(names[i], temp);
        free(temp);
    }

}

void semaphoreInitializer(){
    // Initialization of semaphore
    if( (signalToChef[0] = sem_open(names[0], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (signalToChef[1] = sem_open(names[1], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (signalToChef[2] = sem_open(names[1], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (signalToChef[3] = sem_open(names[2], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (signalToChef[4] = sem_open(names[3], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (signalToChef[5] = sem_open(names[4], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Error while opening semaphore ");
    if( (accessing = sem_open(names[5], O_CREAT,  0644, 1)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");

    if( (milk = sem_open(names[6], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (flour = sem_open(names[7], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (sugar = sem_open(names[8], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (walnut = sem_open(names[9], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");

    if( (milkAndFlourSignal = sem_open(names[10], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (flourAndSugarSignal = sem_open(names[11], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (milkAndSugarSignal = sem_open(names[12], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (walnutAndFlourSignal = sem_open(names[13], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (walnutAndMilkSignal = sem_open(names[14], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (walnutAndSugarSignal = sem_open(names[15], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (dessertPrepared = sem_open(names[16], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
    if( (childReturned = sem_open(names[17], O_CREAT,  0644, 0)) == SEM_FAILED )
        errorAndExit("Semaphore initialize error ");
}

void closeSemaphores(){

    for(int i = 0; i < NUMBER_OF_SEMAPHORES; i++)
        if( sem_unlink(names[i]) == -1 )
            errorAndExit("Error while unlinking semaphores");

    for(int i = 0; i < NUMBER_OF_CHEFS; i++)
        if( sem_close(signalToChef[i]) == -1 )
            errorAndExit("Error while closing signalToChef semaphores");

    if(sem_close(accessing) < 0) errorAndExit("Error while closing accessing semaphore");
    if(sem_close(milk) < 0)       errorAndExit("Error while closing milk semaphore");
    if(sem_close(sugar) < 0)      errorAndExit("Error while closing sugar semaphore");
    if(sem_close(walnut) < 0)     errorAndExit("Error while closing walnut semaphore");
    if(sem_close(flour) < 0)      errorAndExit("Error while closing flour semaphore");

    if(sem_close(milkAndFlourSignal) < 0) errorAndExit("Error while closing milkAndFlourSignal semaphore");
    if(sem_close(flourAndSugarSignal) < 0) errorAndExit("Error while closing flourAndSugarSignal semaphore");
    if(sem_close(milkAndSugarSignal) < 0) errorAndExit("Error while closing milkAndSugarSignal semaphore");
    if(sem_close(walnutAndFlourSignal) < 0) errorAndExit("Error while closing walnutAndFlourSignal semaphore");
    if(sem_close(walnutAndMilkSignal) < 0) errorAndExit("Error while closing walnutAndMilkSignal semaphore");
    if(sem_close(walnutAndSugarSignal) < 0) errorAndExit("Error while closing walnutAndSugarSignal semaphore");

    if(sem_close(dessertPrepared) < 0) errorAndExit("Error while closing dessertPrepared semaphore");
    if(sem_close(childReturned) < 0) errorAndExit("Error while closing childReturned semaphore");

}

void whileExiting(){
    // This function is called when the program is exit() is called
    closeSemaphores();
    free(ingredientsInFile);
    for(int i = 0; i < NUMBER_OF_SEMAPHORES; i++)
        free(names[i]);
    
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
            // dprintf(STDOUT_FILENO, "Pusher id is %d\n", pid);
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
            chef(i);
            return; // If chef(child) is done, return from the child.
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            pidsFromFork[i + NUMBER_OF_INGREDIENTS] = pid;
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process for chefs. ");
        }
    } // End of for loop for creating child processes for chefs

    // Do wholesaler job.
    if(pid > 0){  // parent
        if( atexit(whileExiting) == -1 )
            errorAndExit("Error while exiting");
        wholesalerProcess(pidsFromFork);
        // Collecting chef's return values 
        int totalNumberOfDesserts = 0;

        for(int i =0; i < NUMBER_OF_CHEFS; i++){
            // dprintf(STDOUT_FILENO, "Pid %d\n", pidsFromFork[i]);
            int status;
            int waitpidReturnVal = waitpid(pidsFromFork[NUMBER_OF_INGREDIENTS + i], &status, 0/* Don't wait for child process to finish */);
            if(waitpidReturnVal  < 0)   
                errorAndExit("Error while using waitpid\n");
            if( waitpidReturnVal == pidsFromFork[NUMBER_OF_INGREDIENTS+i] && WIFEXITED(status) ){
                // printf("+-+-Child process %d exited with status %d, WEXITSTATUS %d, status >> 8 is %d, status&0xff %d\n", 
                //                     pid, status, WEXITSTATUS(status), status >> 8, status & 0xFF);
                totalNumberOfDesserts += WEXITSTATUS(status); //  This macro evaluates to the low-order 8 bits of the status. (Same with status >> 8)
            }
        }
        dprintf(STDOUT_FILENO, "\nThe wholesaler (pid %d) is done (Total desserts: %d)\n", getpid(), totalNumberOfDesserts);
    }

}

void wholesalerProcess(pid_t pidsFromFork[]){
    while(TRUE){
        int foundFLag1 = FALSE, foundFLag2 = FALSE;
        // sem_wait(wholesaler);
        switch(sharedMemory->ingredients[0]){
            case 'M':   sem_post(milk);   foundFLag1 = TRUE; break;
            case 'W':   sem_post(walnut); foundFLag1 = TRUE; break;
            case 'F':   sem_post(flour);  foundFLag1 = TRUE; break;
            case 'S':   sem_post(sugar);  foundFLag1 = TRUE; break;
        }
        switch(sharedMemory->ingredients[1]){
            case 'M':   sem_post(milk);   foundFLag2 = TRUE; break;
            case 'W':   sem_post(walnut); foundFLag2 = TRUE; break;
            case 'F':   sem_post(flour);  foundFLag2 = TRUE; break;
            case 'S':   sem_post(sugar);  foundFLag2 = TRUE; break;
        }

        // If a dessert is prepared, then let program continue with next line.
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) delivers %s and %s.\n", getpid(), 
                                stringConverter(sharedMemory->ingredients[0]), stringConverter(sharedMemory->ingredients[1]) );
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) is waiting for the dessert.\n", getpid());
        sem_wait(dessertPrepared);
        dprintf(STDOUT_FILENO, "The wholesaler (pid %d) has obtained the dessert and left.\n", getpid());   

        // Changing with new line for the next case.
        if(foundFLag1 == TRUE && foundFLag2 == TRUE){
            sharedMemory->ingredients[0] = ingredientsInFile[lineNumber][0];
            sharedMemory->ingredients[1] = ingredientsInFile[lineNumber][1];
        }
        // dprintf(STDOUT_FILENO, "--Entering line number %d--\n", lineNumber);
        if(lineNumber == sharedMemory->numberOfLines || signalFlag == 1){
            // Killing the PUSHER processes with SIGKILL
            for(int i = 0; i < NUMBER_OF_INGREDIENTS; i++)
                if( kill(pidsFromFork[i], SIGKILL) == -1 )
                    errorAndExit("Killing child failed.\n");
            // (Gracefully) Killing the CHEF processes with SIGINT
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
                sem_wait(milkAndFlourSignal);
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) has taken the walnut\nChef0 (pid %d) has taken the sugar.\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef0 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            case 1: // Waiting for FLOUR and WALNIT [has milk and sugar]
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) is waiting for milk and sugar.\n", getpid());
                sem_wait(milkAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) has taken the walnut\nChef0 (pid %d) has taken the flour.\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef1 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            case 2: // Waiting for SUGAR and FLOUR [has milk and walnuts]
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) is waiting for sugar and flour.\n", getpid());
                sem_wait(walnutAndMilkSignal);
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) has taken the flour\nChef2 (pid %d) has taken the sugar\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef2 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            case 3: // Waiting for MILK and FLOUR [has sugar and walnuts]
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) is waiting for milk and flour.\n", getpid());
                sem_wait(walnutAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) has taken the flour\nChef3 (pid %d) has taken the milk\n",  getpid(),  getpid());
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef3 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            case 4: // Waiting for  MILK and WALNUTS [has sugar and flour]
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) is waiting for milk and walnuts.\n", getpid());
                sem_wait(flourAndSugarSignal);
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) has taken the milk\nChef4 (pid %d) has taken the walnuts\n", getpid(),  getpid());
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) is preparing the dessert.\n", getpid());
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef4 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            case 5: // Waiting for SUGAR and MILK [has flour and walnuts]
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) is waiting for sugar and milk.\n", getpid());
                sem_wait(walnutAndFlourSignal);
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) has taken the milk\nChef5 (pid %d) has taken the sugar\n", getpid(), getpid());
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) is preparing the dessert.\n", getpid());
                // Dessert is prepared so let program continue with next line of file
                sem_post(dessertPrepared);
                dprintf(STDOUT_FILENO, "Chef5 (pid %d) has delivered the dessert.\n\n", getpid());
                break;
            default:
                errorAndExit("Chef number is not valid. ");
        }
        ++counter;
        if(signalFlag == 1){
            // sem_wait() function is interruptible by a signal. So, last ++counter comes eventually. (--counter is needed)
            --counter; 
            printf("Chef%d prepared %d desserts.\n", chefNumber, counter);
            printf("Chef%d (pid %d) is exiting...\n", chefNumber, getpid());
            // return counter;
            // https://man7.org/linux/man-pages/man3/exit.3.html 
            // As it says here; with exit(), least significant byte of status (i.e., status & 0xFF) is RETURNed to the parent
            // free(ingredientsInFile); //çç
            exit(counter & 0XFF); 
        }
    }
}

/********************** PUSHERS **********************/
void pusherMilk(){ //Wakes up when there is milk by wholesaler
    sem_wait(milk);
    sem_wait(accessing);

    if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(walnutAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(walnutAndFlourSignal);        
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(flourAndSugarSignal);
    }
    else
        sharedMemory->isMilk = 1;

    sem_post(accessing);
}

void pusherFlour(){
    sem_wait(flour);
    sem_wait(accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(walnutAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(walnutAndMilkSignal);
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(milkAndSugarSignal);
    }
    else
        sharedMemory->isFlour = 1;

    sem_post(accessing);
}

void pusherSugar(){
    sem_wait(sugar);
    sem_wait(accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(walnutAndFlourSignal);
    }
    else if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(walnutAndMilkSignal);
    }
    else if(sharedMemory->isWalnut == TRUE){
        sharedMemory->isWalnut = FALSE;
        sem_post(milkAndFlourSignal);
    }
    else
        sharedMemory->isSugar = 1;

    sem_post(accessing);
}

void pusherWalnut(){
    sem_wait(walnut);
    sem_wait(accessing);

    if(sharedMemory->isMilk == TRUE){
        sharedMemory->isMilk = FALSE;
        sem_post(flourAndSugarSignal);
    }
    else if(sharedMemory->isSugar == TRUE){
        sharedMemory->isSugar = FALSE;
        sem_post(milkAndFlourSignal);
    }
    else if(sharedMemory->isFlour == TRUE){
        sharedMemory->isFlour = FALSE;
        sem_post(milkAndSugarSignal);
    }
    else
        sharedMemory->isWalnut = 1;

    sem_post(accessing);
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

char *sg_itoa(int number){
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