target: compile_all

compile_all:
	gcc -Wall main_unnamed.c sync_unnamed.c -o hw3unnamed
	gcc -Wall main_named.c sync_named.c -o hw3named

compile_unnamed:
	gcc -Wall main_unnamed.c sync_unnamed.c -o hw3unnamed

run_unnamed:
	gcc -Wall main_unnamed.c sync_unnamed.c -o hw3unnamed
	./hw3unnamed -i files/inputFile.txt 

compile_named: 
	gcc -Wall main_named.c sync_named.c -o hw3named
	
run_named:
	gcc -Wall main_named.c sync_named.c -o hw3named
	./hw3named -i files/inputFile.txt -n name

run_named_valgrind:
	gcc -g -Wall main_named.c sync_named.c -o hw3named
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes  ./hw3named -i files/inputFile.txt -n name

# Suleyman Golbol 1801042656