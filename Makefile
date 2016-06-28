all:
	gcc server.c database.c utils.c -o server 
	gcc client.c utils.c database.c -o client -lncurses -pthread

