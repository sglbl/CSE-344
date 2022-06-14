target: compile

compile:
	gcc -pthread -Wall -std=gnu17 src/server.c src/common.c -lm -o bin/server
	gcc -pthread -Wall -std=gnu17 src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -Wall -std=gnu17 src/client.c src/common.c -lm -o bin/client
# ./bin/servant -d dataset/HATAY -c 1-9 -r 127.0.0.1 -p 33000
# ./bin/server -p 33000 -t 11
# ./bin/client -r ../requestFile -q 33000 -s 127.0.0.1

clean:
	rm -f bin/server bin/servant bin/client

for_debug:
	gcc -pthread -g -Wall src/server.c src/common.c -lm -o bin/server
	gcc -pthread -g -Wall src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -g -Wall src/client.c src/common.c -lm -o bin/client

debug:
	gcc -pthread -Wall -g src/server.c src/common.c -lm -o bin/server
	gcc -pthread -Wall -g src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -Wall -g src/client.c src/common.c -lm -o bin/client
	gdb -q ./bin/client
# r -d dataset/HATAY -c 1-9 -r 127.0.0.1 -p 33000

valgrind:
	gcc -pthread -Wall src/server.c src/common.c -lm -o bin/server
	gcc -pthread -Wall src/servant.c src/common.c -lm -o bin/servant
	gcc -pthread -Wall src/client.c src/common.c -lm -o bin/client
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./bin/server -i files/file1.txt -j files/file2.txt -o output -n 4 -m 2

# Suleyman Golbol 1801042656