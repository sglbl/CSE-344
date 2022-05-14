target: compile

compile:
	gcc -Wall *.c -lm -o hw5

run:
	gcc -Wall *.c -lm -o hw5
	./hw5 -i files/file1.txt -j files/file2.txt -o output -n 4 -m 2

debug:
	gcc -Wall -g *.c -lm -o hw5
	gdb -q ./hw5
# r -i files/file1.txt -j files/file2.txt -o output -n 4 -m 2

run_warning:
	gcc -Wall -Wextra *.c -o hw4
	./hw4 -C 10 -N 5 -F files/file.txt

clean:
	rm -f hw4

run_valgrind:
	gcc -g -Wall *.c -o hw4
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes  ./hw4 -C 10 -N 5 -F files/file2.txt

# Suleyman Golbol 1801042656