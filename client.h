
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include "utils.h"
	
#define MAX_LINE 256

void *SendMessage();
void *RecvMessage();
int GetWhoMessage (char str[], char buf[]);

