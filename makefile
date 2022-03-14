target: compile

compile:main.c sg_replacer.c sg_replacer.h
	gcc main.c sg_replacer.c -lm -Wall -o hw1

run:		# Example usage while executing --> make run args="'/str1/str2/i;/str3/str4/i' files/ex3.txt"
	gcc main.c sg_replacer.c -lm -Wall -o hw1
	./hw1 $(args)	

debug:
	gcc main.c sg_replacer.c -g -lm -Wall -o hw1
	gdb -q ./hw1 $(args)

warnings:
	gcc main.c library.c -lm -Wall -Wextra -o hw1
	./hw1

#Suleyman Golbol 1801042656