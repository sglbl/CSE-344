target: compile

compile: main.c
	gcc -Wall main.c clientX.c -o client
	gcc -Wall serverY.c -lrt -pthread -o serverY 
# ./client -s serverFifo.txt -o files/data.csv
# ./serverY -s serverFifo.txt -o logFile.txt -p 5 -r 4 -t 2

# ---------- Killing commands --------------
# KILLING ZOMBIES
# kill -9 $(ps -A -ostat,ppid | grep -e '[zZ]'| awk '{ print $2 }')
# KILLING DAEMON (searching daemons of serverY and list their pid's ad kill them all with their pids.)
# kill -9 $(ps -C serverY -o pid=)


# Suleyman Golbol 1801042656