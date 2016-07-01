
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
    
    scrollok(display, TRUE);
    idlok(display, TRUE);
    
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

	int i, error, id, ok, fdbk = 0;
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

	memset(str, '\0', MAX_LINE);
	snprintf(str, MAX_LINE, "%d %s", login_msg, name);
	send(s, str, strlen(str)+1, 0);

	while (online) {

		error = 0;
        memset(str,'\0', MAX_LINE);
		memset(buf,'\0', MAX_LINE);
		mvwgetstr(input, 1, 0, str);	
		str[MAX_LINE-1] = '\0';		
		messageType type = GetMessageType(getNWord(str, 1));

		if (type == new_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s \"%s\"", new_msg, id, getNWord(str,2), getNWord(str,0));

		} else if (type == create_group) {
			snprintf(buf, MAX_LINE, "%d %s", create_group, getNWord(str,2));		
		
		} else if (type == join_group) {
			snprintf(buf, MAX_LINE, "%d %s", join_group, getNWord(str,2));
		
		} else if (type == newgrp_msg) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s \"%s\"", newgrp_msg, id, getNWord(str,2), getNWord(str,3));
		
		} else if (type == who_msg) {
			snprintf(buf, MAX_LINE, "%d", who_msg);

		} else if (type == exit_msg) {
			snprintf(buf, MAX_LINE, "%d", exit_msg);
			online = 0;

		} else if (type == invite_player) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s", invite_player, id, getNWord(str,2));
		
		} else if (type == accept_invite) {
			snprintf(buf, MAX_LINE, "%d %s", accept_invite, getNWord(str,2));
		
		} else if (type == refuse_invite) {
			snprintf(buf, MAX_LINE, "%d %s", refuse_invite, getNWord(str,2));

		} else if (type == game_move) {
			id = hash(str);
			snprintf(idstr, 10, "%d", id);
			snprintf(buf, MAX_LINE, "%d %d %s %s %s", game_move, id, getNWord(str,2), getNWord(str,3), getNWord(str,4));
			
		} else {
			error = 1;
			snprintf(buf, MAX_LINE, "Comando inválido: %s!", str);					
			strcpy(str,buf);
		}
		
		pthread_mutex_lock(&display_mutex);
		ScrollBeforeDisplay(1);
		mvwprintw(display, k, 0, "$[%s] %s", name, str);
		ScrollAfterDisplay(1);
		wrefresh(display);	
		wclear(input);
		mvwprintw(input, 0, 0, "%s", divisory);
		mvwprintw(input, 0, 0, "$[%s]", name);
		wrefresh(input);
		pthread_mutex_unlock(&display_mutex);

		if (!error)
			ok = send(s, buf, strlen(buf)+1, 0);
		
		if (ok && (type == new_msg || type == newgrp_msg)) {
			pthread_mutex_lock(&display_mutex);
			ScrollBeforeDisplay(1);
			mvwprintw(display, k, 0, "Mensagem #%s enviada para o servidor!\n", idstr);
			ScrollAfterDisplay(1);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);
		}
		
		if (ok && (type == invite_player)) {
			pthread_mutex_lock(&display_mutex);
			ScrollBeforeDisplay(1);
			mvwprintw(display, k, 0, "Pedido #%s enviado para o servidor!\n", idstr);
			ScrollAfterDisplay(1);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);				
		}
		
		if (ok && (type == game_move)) {
			pthread_mutex_lock(&display_mutex);
			ScrollBeforeDisplay(1);
			mvwprintw(display, k, 0, "Movimento #%s enviado para o servidor!\n", idstr);
			ScrollAfterDisplay(1);
			wrefresh(display);
			pthread_mutex_unlock(&display_mutex);				
		}

		if (ok && (type == accept_invite || type == refuse_invite)) {
			pthread_mutex_lock(&display_mutex);
			ScrollBeforeDisplay(1);
			mvwprintw(display, k, 0, "Resposta de jogo enviada para o servidor!\n");
			ScrollAfterDisplay(1);
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
		lines = 1;
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
			snprintf(buf, MAX_LINE, "Mensagem #%s recebida!\n", getNWord(str,2));
	
		} else if (type == create_group) {
			snprintf(buf, MAX_LINE, "Grupo [@%s] criado com sucesso!\n", getNWord(str,2));

		} else if (type == join_group) {
			snprintf(buf, MAX_LINE, "Usuário [%s] adicionado ao grupo [@%s] com sucesso!\n", getNWord(str,3), getNWord(str,2));

		} else if (type == invite_player) {
			snprintf(buf, MAX_LINE, "Usuário [%s] está desafiando você para uma partida de Jogo da Velha!\n", getNWord(str,3));
			snprintf(aux, MAX_LINE, "%d %s %s", invite_received, getNWord(str,2), getNWord(str,3));
			send(s, aux, strlen(aux)+1, 0);

		} else if (type == invite_sent) {
			snprintf(buf, MAX_LINE, "Pedido #%s enfileirado!\n", getNWord(str,2));

		} else if (type == invite_received) {
			snprintf(buf, MAX_LINE, "Pedido #%s recebido!\n", getNWord(str,2));

		} else if (type == accept_invite) {
			snprintf(buf, MAX_LINE, "Usuário [%s] aceitou seu pedido para jogar o Jogo da Velha!\n", getNWord(str,2));
			lines += GetGameMessage("\0",aux, getNWord(str,2));
			strcat (buf,aux);

		} else if (type == refuse_invite) {
			snprintf(buf, MAX_LINE, "Usuário [%s] recusou seu pedido para jogar o Jogo da Velha!\n", getNWord(str,2));

		} else if (type == game_move) {
			lines = GetGameMessage(str,buf, getNWord(str,3));			
			snprintf(aux, MAX_LINE, "%d %s %s", move_received, getNWord(str,2), getNWord(str,3));
			send(s, aux, strlen(aux)+1, 0);

		} else if (type == move_sent) {
			snprintf(buf, MAX_LINE, "Movimento #%s enfileirado!\n", getNWord(str,2)); 
			
		} else if (type == move_received) {
			snprintf(buf, MAX_LINE, "Movimento #%s recebido!\n", getNWord(str,2)); 
			
		} else if (type == won_game) {
			snprintf(buf, MAX_LINE, "Parabéns, você venceu o jogo contra [%s]!\n", getNWord(str,2)); 

		} else if (type == lost_game) {
			snprintf(buf, MAX_LINE, "Oh não, você perdeu o jogo contra [%s]!\n", getNWord(str,2)); 

		} else if (type == draw_game) {
			snprintf(buf, MAX_LINE, "Mas que loucura, o jogo contra [%s] empatou!\n", getNWord(str,2)); 

		} else if (type == who_msg) {
			lines = GetWhoMessage(str,buf);			
									
		} else if (type == error_msg)
			snprintf(buf, MAX_LINE, "Erro: %s", getNWord(str,0));			
		
		pthread_mutex_lock(&display_mutex);
		ScrollBeforeDisplay(lines);
		mvwprintw(display, k, 0, "%s", buf);
		ScrollAfterDisplay(lines);		
		wrefresh(display);
		wrefresh(input);
		pthread_mutex_unlock(&display_mutex);
	}

}

int GetGameMessage (char str[], char buf[], char player[]) {
	
	char aux[MAX_LINE];
	char *mark = getNWord(str,0);
	
	if (!strcmp(str,"\0"))
		mark = "         ";
		
	snprintf (buf, MAX_LINE, "Status da partida de Jogo da Velha com [%s]:\n", player);	
	snprintf (aux, MAX_LINE, "  A | B | C \n"); 
	strcat(buf,aux);
	snprintf (aux, MAX_LINE, "1 %c | %c | %c \n", mark[0], mark[1], mark[2]); 
	strcat(buf,aux);
	snprintf (aux, MAX_LINE, " ---|---|---\n"); 
	strcat(buf,aux);
	snprintf (aux, MAX_LINE, "2 %c | %c | %c \n", mark[3], mark[4], mark[5]); 
	strcat(buf,aux);
	snprintf (aux, MAX_LINE, " ---|---|---\n"); 
	strcat(buf,aux);
	snprintf (aux, MAX_LINE, "3 %c | %c | %c \n", mark[6], mark[7], mark[8]); 
	strcat(buf,aux);		

	return 7;

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

	for (i = 0; i <= users; i++) {
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

	return users+1;
	
}

void ScrollBeforeDisplay (int n) {

	if (k+n-1 > LINES-3) {
		int up = n-((LINES-3)-k);
		k = k-up;
		wscrl(display, up);		
	}
	
}

void ScrollAfterDisplay (int n) {

	k += n;
	
}
