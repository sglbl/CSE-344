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

warning:
	gcc -Wall -Wextra *.c -lm -o hw5

clean:
	rm -f hw5

run_valgrind:
	gcc -Wall *.c -g -lm -o hw5
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i files/file1.txt -j files/file2.txt -o output -n 4 -m 2

# Suleyman Golbol 1801042656