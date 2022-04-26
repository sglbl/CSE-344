target: compile

compile:
	gcc -Wall main_unnamed.c sync_unnamed.c -o hw3unnamed

run:
	gcc -Wall main_unnamed.c sync_unnamed.c -o hw3unnamed
	./hw3unnamed -i files/inputFile.txt 

compile_named: 
	gcc -Wall main_named.c sync_named.c -o hw3named
	
run_named:
	gcc -Wall main_named.c sync_named.c -o hw3named
	./hw3named -i files/inputFile.txt -n name

# Suleyman Golbol 1801042656