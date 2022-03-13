target: compile

compile:main.c sg_replacer.c sg_replacer.h sg_expression_matcher.c sg_expression_matcher.h
	gcc main.c sg_replacer.c sg_expression_matcher.c -lm -Wall -o hw1

run:		# Example usage while executing --> make run args="1 2 3"
	gcc main.c sg_replacer.c sg_expression_matcher.c -lm -Wall -o hw1
	./hw1 $(args)	

debug:
	gcc main.c sg_replacer.c sg_expression_matcher.c -g -lm -Wall -o hw1
	gdb -q ./hw1 $(args)

warnings:
	gcc main.c library.c -lm -Wall -Wextra -o hw1
	./hw1

#Suleyman Golbol 1801042656