
#include "client.h"

int online = 0;
volatile int k = 0;
int s;
char* name;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
WINDOW *display;
WINDOW *input;

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
    
    scrollok(input, TRUE);
    scrollok(display, TRUE);
    
    pthread_t recv_thread;
    pthread_t send_thread;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&recv_thread, &attr, SendMessage, NULL);
    pthread_create(&send_thread, &attr, RecvMessage, NULL);

    while(online != -1);
    
   	delwin(display);
	delwin(input);
	endwin();
	close(s);
	
}

void *SendMessage () {

	int i, id, ok, fdbk = 0;
	char buf[MAX_LINE];
	char str[MAX_LINE];
	char *aux;
	char idstr[10];
	char divisory[COLS];
	
	// imprime layout da tela do usuario

	for (i = 0; i < COLS; i++)
        divisory[i] = '-';

	pthread_mutex_lock(&display_mutex);
	mvwprintw(input, 0, 0, "%s\n", divisory);
	mvwprintw(input, 0, 0, "$[%s]", name);
	wrefresh(display);
	wrefresh(input);
	pthread_mutex_unlock(&display_mutex);

	// envia notificacao de login para o server	

	memset(str, '\0', MAX_LINE);
	snprintf(str, MAX_LINE, "%d %s", login_msg, name);
	send(s, str, strlen(str)+1, 0);

	while (online) {

		// recebe e trata entrada do usuario
		// manda mensagem para o servidor
		// imprime feedback para o usuario		

        memset(str,'\0', MAX_LINE);
		memset(buf,'\0', MAX_LINE);
		mvwgetstr(input, 1, 0, str);	
		str[MAX_LINE-1] = '\0';
		
		messageType type = GetMessageType(getNWord(str, 1));

		if (type == new_msg) {
			fdbk = 1;
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s", new_msg, id, str+5);

		} else if (type == create_group) {
			snprintf(buf, MAX_LINE, "%d %s", create_group, str+8);		
		
		} else if (type == join_group) {
			snprintf(buf, MAX_LINE, "%d %s", join_group, str+6);
		
		} else if (type == newgrp_msg) {
			fdbk = 1;
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s", newgrp_msg, id, str+6);
		
		} else if (type == who_msg) {
			snprintf(buf, MAX_LINE, "%d", who_msg);

		} else if (type == exit_msg) {
			snprintf(buf, MAX_LINE, "%d", exit_msg);
			online = 0;

		} else if (type == game_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %s", game_msg, str+5);
			//CreateGame(str+5);	
		
		} else if (type == acptg_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %s", game_msg, str+8);
			//CreateGame(str+5);				
		
		} else if (type == rfsg_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %s", game_msg, str+7);

		} else if (type == play_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %s", play_msg, str+6);
			//UpdateGame(str+6);
			
		} else {
			snprintf(buf, MAX_LINE, "%d", error_msg);
			snprintf(str, MAX_LINE, "That's an invalid command, sorry!");					
		}
		
		pthread_mutex_lock(&display_mutex);
		mvwprintw(display, k++, 0, "$[%s] %s", name, str);
		wrefresh(display);	
		pthread_mutex_unlock(&display_mutex);
		wclear(input);
		mvwprintw(input, 0, 0, "%s", divisory);
		mvwprintw(input, 0, 0, "$[%s]", name);
		wrefresh(input);

		ok = send(s, buf, strlen(buf)+1, 0);
		
		if (ok && (type == new_msg || type == newgrp_msg)) {
			pthread_mutex_lock(&display_mutex);
			mvwprintw(display, k++, 0, "Mensagem #%s enviada!\n", idstr);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);
		}
		
		if (type == game_msg) {
			pthread_mutex_lock(&display_mutex);
			mvwprintw(display, k++, 0, "Pedido #%s enviado!\n", idstr);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);				
		}
		
		if (type == play_msg) {
			pthread_mutex_lock(&display_mutex);
			mvwprintw(display, k++, 0, "Movimento #%s enviado!\n", idstr);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);				
		}

		if (type == acptg_msg || type == rfsg_msg || type == acptf_msg || type == rfsf_msg) {
			pthread_mutex_lock(&display_mutex);
			mvwprintw(display, k++, 0, "Resposta #%s enviada!\n", idstr);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);				
		}


	}
	
	online = -1;
	
}

void *RecvMessage () {

	int i, lines;
	char str[MAX_LINE];
	char buf[MAX_LINE];
	char aux[MAX_LINE];
	char divisory[COLS];
	messageType type;
	
	for (i = 0; i < COLS; i++)
        divisory[i] = '-';

	while(online) {

    	memset(str,'\0', MAX_LINE);
		memset(buf,'\0', MAX_LINE);
		memset(aux,'\0', MAX_LINE);
		recv(s, str, MAX_LINE, 0);
		lines = 0;
		type = (messageType) atoi(getNWord(str,1));

		if (type == new_msg) {
			snprintf(buf, MAX_LINE, "[%s>] %s", getNWord(str,3), getNWord(str,0));
			snprintf(aux, MAX_LINE, "%d %s %s", recv_msg, getNWord(str,2), getNWord(str,3));
			send(s, aux, strlen(aux)+1, 0);
		
		} else if (type == newgrp_msg) {
			snprintf(buf, MAX_LINE, "[%s@%s>] %s", getNWord(str,3), getNWord(str,4), getNWord(str,0));
			snprintf(aux, MAX_LINE, "%d %s %s %s", recvgrp_msg, getNWord(str,2), getNWord(str,3), getNWord(str,4));
			send(s, aux, strlen(aux)+1, 0);

		} else if (type == sent_msg) {
			snprintf(buf, MAX_LINE, "Mensagem #%s enfileirada!\n", getNWord(str,2));
		
		} else if (type == recv_msg) {
			snprintf(buf, MAX_LINE, "Mensagem #%s recebida por %s!\n", getNWord(str,2), getNWord(str,3));		
	
		} else if (type == create_group) {
			snprintf(buf, MAX_LINE, "Grupo %s criado com sucesso!\n", getNWord(str,2));

		} else if (type == join_group) {
			snprintf(buf, MAX_LINE, "Adicionado ao grupo %s com sucesso!\n", getNWord(str,2));

/*		} else if (type == game_msg) {
			snprintf(buf, MAX_LINE, "Usuário [%s] está desafiando você para uma partida de Jogo da Velha!\n", getNWord(str,2));


		} else if (type == acptg_msg) {
			snprintf(buf, MAX_LINE, "Usuário [%s] aceitou jogar Jogo da Velha, sua vez de jogar!\n", getNWord(str,2));
			//ShowGame();

		} else if (type == play_msg) {
			snprintf(buf, MAX_LINE, "Usuário [%s] realizou um movimento no Jogo da Velha, sua vez de jogar!\n", getNWord(str,2));
			//ShowGame();

		} else if (type == sentg_msg) {
			snprintf(buf, MAX_LINE, "Pedido #%s enfileirado!\n", getNWord(str,2));
		
		} else if (type == recvg_msg) {
			snprintf(buf, MAX_LINE, "Pedido #%s recebidao por %s!\n", getNWord(str,2), getNWord(str,3));		

		} else if (type == sentp_msg) {
			snprintf(buf, MAX_LINE, "Movimento #%s enfileirado!\n", getNWord(str,2));
		
		} else if (type == recvp_msg) {
			snprintf(buf, MAX_LINE, "Movimento #%s recebido por %s!\n", getNWord(str,2), getNWord(str,3));		
*/
		} else if (type == who_msg) {
			lines = GetWhoMessage(str, buf);			
									
		} else
			continue;
		
		pthread_mutex_lock(&display_mutex);
		mvwprintw(display, k++, 0, "%s", buf);
		wrefresh(display);
		k += lines;
		pthread_mutex_unlock(&display_mutex);
	}

}

int GetWhoMessage (char str[], char buf[]) {

	char aux[MAX_LINE];
	char *who;
	char *users_names;
	int i, on, users, size;

	users_names = getNWord(str,0);
	users = atoi(getNWord(users_names,1));
	size = atoi(getNWord(users_names,users+3));
	
	if (size < 7)
		size = 7;

	snprintf (buf, MAX_LINE, "| %-*s | status  |\n", size, "usuario");
	on = 1;

	for (i = 0; i < users; i++) {
		who = getNWord(users_names,i+2);
		if (!strcmp(who,"|"))
			on = 0;
		else if (on) {
			snprintf(aux, MAX_LINE, "| %-*s | online  |\n", size, who);
			strcat(buf, aux); 										
		} else if (!on) {
			snprintf(aux, MAX_LINE, "| %-*s | offline |\n", size, who);
			strcat(buf, aux); 														
		}	
	}

	return users;
	
}
