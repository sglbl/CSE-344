#ifndef PREREQUISITES_H_
#define PREREQUISITES_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0
#define NO_EXIT 1

#define TEMP_PID "/tmp/singleton.pid"
/* A template name for client fifo */
#define TEMP_CLIENT_FIFO "/tmp/client_fifo"
/* Client fifo path name space */
#define CLIENT_FIFO_PATH_NAME_SIZE sizeof(TEMP_CLIENT_FIFO) + 10

typedef struct ClientRequest {
    pid_t pid;
    int sequenceLength;
    int rowSize;
    int totalMatrixSize;
    int matrix[];
} ClientRequest;

typedef struct ServerResponse {
    int serverPid;
    int sequenceNumber;
    int isInvertible;
} ServerResponse;

typedef struct SharedMemory {
    int poolSizeZ;
    int sleepTime;
    int isRunning;
    int isThereRequest;
    // int available;
    ClientRequest clientRequest;
} SharedMemoryForZ;

#endif