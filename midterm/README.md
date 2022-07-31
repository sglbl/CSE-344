Simulating 2 process-pooled servers (Y and Z) executing on the same system as the clients. 
 Which;
  * Y and Z are daemon processes that have no controlling terminal.
  * Y is singleton (measure against double instantiation).
  * Y have p children processes (workers).
  * If children of Y are already busy, they send requests to Z through pipe.
  * Z is a server instantiated by Server Y.
  * Z have children process pool (workers) and delegates requests to its children using shared memory segment.
  * Client connects to server Y through the server fifo (given as a commandline argument), sends its request, receives its response through its client fifo, prints it to STDOUT and exit. (Also it exits gracefully in case of SIGINT)
