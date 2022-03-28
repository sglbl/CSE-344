target: compile

compile: main.c sg_process_p.h sg_process_r.h
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
# Valgrind example: valgrind --leak-check=full --show-reachable=yes ./processP -i files/inputFile.dat -o files/outputFile.dat
	
run:
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
	./processP $(args)
# Example usage while executing --> [Note: "make run args" doesn't support using dollar sign within ]
# make run args="-i inputFilePath -o outputFilePath"
# make run args="-i files/inputFile.dat -o files/outputFilePath.dat"

debug:
	gcc -g *.c -Wall -o processP

valgrind:
	gcc -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -g -Wall sg_matrix.c sg_process_r.c -lm -o processR
	valgrind --leak-check=full --show-reachable=yes ./processP $(args)

# 

fsanitize:
	gcc -fsanitize=address -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -fsanitize=address -g -Wall sg_matrix.c sg_process_r.c -lm -o processR

fsanitizeAndRun:
	gcc -fsanitize=address -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -fsanitize=address -g -Wall sg_matrix.c sg_process_r.c -lm -o processR
	./processP $(args)
# make fsanitizeAndRun args="-i files/inputFile.dat -o files/outputFile.dat"

warnings:
	gcc *.c -lm -Wall -Wextra -o processP
	./processP

# Suleyman Golbol 1801042656