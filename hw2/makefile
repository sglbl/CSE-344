target: compile

compile: main.c sg_process_p.h sg_process_r.h
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
# ./processP -i files/inputFile.dat -o files/outputFilePath.dat
	
run:
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
	./processP $(args)
# make run args="-i inputFilePath -o outputFilePath"
# make run args="-i files/inputFile.dat -o files/outputFilePath.dat"

debug:
	gcc -g *.c -Wall -o processP

valgrind:
	gcc -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -g -Wall sg_matrix.c sg_process_r.c -lm -o processR
	valgrind --leak-check=full --show-reachable=yes ./processP $(args)	
# make valgrind args="-i files/inputFile.dat -o files/outputFile.dat"

valgrindTraceChildren:
	gcc -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -g -Wall sg_matrix.c sg_process_r.c -lm -o processR
	valgrind --trace-children=yes --leak-check=full --show-reachable=yes ./processP $(args)
# make valgrind args="-i files/inputFile.dat -o files/outputFile.dat"

fsanitize:
	gcc -fsanitize=address -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -fsanitize=address -g -Wall sg_matrix.c sg_process_r.c -lm -o processR

fsanitizeAndRun:
	gcc -fsanitize=address -g -Wall main.c sg_process_p.c -lm -o processP 
	gcc -fsanitize=address -g -Wall sg_matrix.c sg_process_r.c -lm -o processR
	./processP $(args)
# make fsanitizeAndRun args="-i files/inputFile.dat -o files/outputFile.dat"

# Suleyman Golbol 1801042656