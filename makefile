target: compile

compile: main.c main.c sg_process.h
	gcc *.c -Wall -o processP
	
run:
	gcc *.c -Wall -o processP
	./processP $(args)
# Example usage while executing --> [Note: "make run args" doesn't support using dollar sign within ]
# make run args="-i inputFilePath -o outputFilePath"
# make run args="-i files/inputFile.dat -o files/outputFilePath.txt"

debug:
	gcc -g *.c -Wall -o processP

debug2:
	gcc -fsanitize=address -g *.c -Wall -o processP

warnings:
	gcc *.c -lm -Wall -Wextra -o processP
	./processP

# Suleyman Golbol 1801042656