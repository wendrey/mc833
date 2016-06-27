#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <ncurses.h>
#include <string.h>

#define MAX_LINE 256

int online = 0;
volatile int k = 0;
int s;
char* name;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
WINDOW *display;
WINDOW *input;

void *SendMessage();
void *RecvMessage();

int main(int argc, char * argv[]) {

    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    int port;
    
    if (argc==4) {
        host = argv[1];
    	port = atoi(argv[2]);
    	name = argv[3];
    }
    else {
        fprintf(stderr, "Error: Usage is ./client <ip> <port> <name>\n");
        exit(1);
    }
    
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "Error: Unknown host: %s\n", host);
        exit(1);
    }
    
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(port);
	
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: Function call socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("Error: Function call connect");
        close(s);
        exit(1);
    }
    
    online = 1;
    
    int ch;
    char str[256];
    char divisory[256];

    initscr();
    nocbreak();
    echo();

    display = newwin(LINES-2, COLS, 0, 0);
    input = newwin(3, COLS, LINES-2, 0);
    
    pthread_t recv_thread;
    pthread_t send_thread;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&recv_thread, &attr, SendMessage, NULL);
    pthread_create(&send_thread, &attr, RecvMessage, NULL);

    while(online);

}

void *SendMessage () {

	int i = 0;
	char str[MAX_LINE];
	char divisory[COLS];
	
	for (i = 0; i < COLS; i++)
        divisory[i] = '-';

	pthread_mutex_lock(&display_mutex);
	mvwprintw(input, 0, 0, "%s\n", divisory);
	wrefresh(display);
	wrefresh(input);
	pthread_mutex_unlock(&display_mutex);

	memset(str, '\0', MAX_LINE);
	strcat(str, "LOGIN ");
	strcat(str, name);
	str[MAX_LINE] = '\0';
	send(s, str, strlen(str)+1, 0);

	while (1) {
        	memset(str,'\0', MAX_LINE);
		mvwgetstr(input, 1, 0, str);	
		str[MAX_LINE-1] = '\0';
		if (str[0] == '\0')
			continue;
		pthread_mutex_lock(&display_mutex);
		mvwprintw(display, k++, 0, "%s", str);
		wclear(input);
		mvwprintw(input, 0, 0, "%s", divisory);
		wrefresh(display);	
		wrefresh(input);
		pthread_mutex_unlock(&display_mutex);
		send(s, str, strlen(str)+1, 0);
	}
	
}

void *RecvMessage () {

	int i;
	char str[MAX_LINE];
	char divisory[COLS];
	
	for (i = 0; i < COLS; i++)
        divisory[i] = '-';

	pthread_mutex_lock(&display_mutex);
	mvwprintw(input, 0, 0, "%s\n", divisory);
	wrefresh(display);
	wrefresh(input);
	pthread_mutex_unlock(&display_mutex);

	while(1) {
        	memset(str,'\0', MAX_LINE);
		recv(s, str, MAX_LINE, 0);
		pthread_mutex_lock(&display_mutex);
		mvwprintw(display, k++, 0, "%s", str);
		wrefresh(display);
		pthread_mutex_unlock(&display_mutex);
	}

}

