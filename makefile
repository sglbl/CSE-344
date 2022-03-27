target: compile

compile: main.c sg_process_p.h sg_process_r.h
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
	
run:
	gcc -Wall main.c sg_process_p.c -lm -o processP 
	gcc -Wall sg_matrix.c sg_process_r.c -lm -o processR
	./processP $(args)
# Example usage while executing --> [Note: "make run args" doesn't support using dollar sign within ]
# make run args="-i inputFilePath -o outputFilePath"
# make run args="-i files/inputFile.dat -o files/outputFilePath.dat"

debug:
	gcc -g *.c -Wall -o processP

debug2:
	gcc -fsanitize=address -g *.c -Wall -o processP

warnings:
	gcc *.c -lm -Wall -Wextra -o processP
	./processP

# Suleyman Golbol 1801042656