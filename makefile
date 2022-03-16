target: compile

compile:main.c sg_replacer.c sg_replacer.h
	gcc main.c sg_replacer.c -Wall -o hw1

run:		
	gcc main.c sg_replacer.c -Wall -o hw1
	./hw1 $(args)	
# Example usage while executing --> [Note: "make run args" doesn't support using dollar sign within :( ]
# make run args="'/str1/str2/i;/str3/str4/i' files/ex3.txt"
# make run args="'/s[tl]r1/str2/i;/sTR3/str4/i;/^str5/str6/' files/ex3.txt"
# make run args="'/s[tl]r1/str2/i;/sTR3/str4/i;/^st[lr]5/str6/' files/ex3.txt"

debug:
	gcc main.c sg_replacer.c -g -lm -Wall -o hw1
	gdb -q ./hw1 $(args)

warnings:
	gcc main.c library.c -lm -Wall -Wextra -o hw1
	./hw1

#Suleyman Golbol 1801042656