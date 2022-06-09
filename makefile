target: compile

compile:
	gcc -pthread -Wall src/server.c src/common.c -lm -o bin/server
	gcc -pthread -Wall src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -Wall src/client.c src/common.c -lm -o bin/client

clean:
	rm -f server servant client

run:
	gcc -pthread -Wall src/server.c src/common.c -lm -o server
	gcc -pthread -Wall src/servant.c src/common.c -lm -o servant
	gcc -pthread -Wall src/client.c src/common.c -lm -o client
# ./bin/servant -d dataset/HATAY -c 1-9 -r 127.0.0.1 -p 33000
# ./server -p 33000 -t 11

debug:
	gcc -pthread -Wall -g src/server.c src/common.c -lm -o bin/server
	gcc -pthread -Wall -g src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -Wall -g src/client.c src/common.c -lm -o bin/client
	gdb -q ./bin/servant
# r -d dataset/HATAY -c 1-9 -r 127.0.0.1 -p 33000


valgrind:
	gcc -pthread -Wall *.c -g -lm -o hw5
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i files/file1.txt -j files/file2.txt -o output -n 4 -m 2

# Suleyman Golbol 1801042656