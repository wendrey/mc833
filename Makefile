all:
	gcc server.c database.c utils.c -o server 
	gcc client.c utils.c -o client -lncurses -pthread

