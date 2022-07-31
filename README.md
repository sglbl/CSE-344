## Assignments of CSE-344 / System Programming on POSIX Systems

### Homework 1
String replacement with low-level system calls.

### Homework 2
Simulating parent-child process relationship. Parent process reads input coordinates from file and forward them for calculations to children processes.

### Homework 3
Simulating bakers synchronization problem using named and unnamed semaphores. (inspired by cigarette smokers synchronization problem) 

### Homework 4
Simulating supplier thread - consumer threads relationship using System V semaphores / semaphore sets.

Using;
  * semget => creating a semaphore set (multiple semaphores)
  * semctl => initializing semaphore set indexes to a number (Using SemUnion)
  * semctl => Checking value of semaphore set index.
  * semop  => Incrementing/decrementing the value of semaphore (or multiple semaphores together) using struct sembuf.
  * tprintf=> Variadic function which uses snprintf() and vprintf() to act like a printf with timestamp.
  * detached thread (for supplier) and joinable threads (for clients)

### Homework 5
 Simulating the usage of POSIX threads to parallelize a couple of mathematical tasks.
 
### Midterm Project
 Simulating 2 process-pooled servers (Y and Z) executing on the same system as the clients. 
 Which;
  * Y and Z are daemon processes that have no controlling terminal.
  * Y is singleton (measure against double instantiation).
  * Y have p children processes (workers).
  * If children of Y are already busy, they send requests to Z through pipe.
  * Z is a server instantiated by Server Y.
  * Z have children process pool (workers) and delegates requests to its children using shared memory segment.
  * Client connects to server Y through the server fifo (given as a commandline argument), sends its request, receives its response through its client fifo, prints it to STDOUT and exit. (Also it exits gracefully in case of SIGINT)

### Final Project
  Simulating a connection system wihch have 3 programs. The servant processes will answer the requests coming from the server through sockets. The client will make requests to the server through sockets, and the server will respond to those requests via the information acquired from the servants. (Using  ip address and unique port numbers)
