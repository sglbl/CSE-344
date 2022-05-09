target: compile

compile:
	gcc -Wall main.c additional.c -o hw4

run:
	gcc -Wall *.c -o hw4
	./hw4 -C 10 -N 5 -F files/file.txt

clean:
	rm -f hw4

run_valgrind:
	gcc -g -Wall *.c -o hw4
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes  ./hw4 -C 10 -N 5 -F inputfilePath

# Suleyman Golbol 1801042656