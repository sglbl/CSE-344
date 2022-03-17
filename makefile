target: compile

compile:main.c sg_replacer.c sg_replacer.h
	gcc main.c sg_replacer.c -Wall -o hw1
# Example usage while executing
# make
# ./hw1 '/s[tl]r1/str2/i;/sTR3/str4/i;/^st[lr]5/str6/;/str7$/str8/' files/ex1.txt
# ./hw1 '/st*r9/str10/' files/ex2.txt
# ./hw1 '/^s[lt]*r9$/str10/' files/ex2.txt
# ./hw1 '/^Window[sz]*/Linux/i;/close[dD]$/open/' files/ex3.txt

run:		
	gcc main.c sg_replacer.c -Wall -o hw1
	./hw1 $(args)	
# Example usage while executing --> [Note: "make run args" doesn't support using dollar sign within ]
# make run args="'/str1/str2/i;/str3/str4/i' files/ex1.txt"
# make run args="'/s[tl]r1/str2/i;/sTR3/str4/i;/^str5/str6/' files/ex1.txt"
# make run args="'/s[tl]r1/str2/i;/sTR3/str4/i;/^st[lr]5/str6/' files/ex1.txt"

debug:
	gcc main.c sg_replacer.c -g -Wall -o hw1

warnings:
	gcc main.c library.c -lm -Wall -Wextra -o hw1
	./hw1

#Suleyman Golbol 1801042656