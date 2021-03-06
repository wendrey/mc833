
#include "server.h"

session client_sessions[MAX_SESSIONS];

int main(int argc, const char * argv[]) {
    
    struct sockaddr_in socket_addr;
    int listening_socket;
    int listening_socket_port;
    int i;

    for (i = 0; i < MAX_SESSIONS; i++) {
        client_sessions[i].socket_id = -1;
    }
    
    setupDatabase();
    
    fd_set allSocketsSet, socketsToRead;
    int highestSocketID;
    
    if (argc == 2) {
        listening_socket_port = atoi(argv[1]);
        if (listening_socket_port <= 1000) {
            perror("Error: Invalid Port");
        }
    } else {
        fprintf(stderr, "Error: Usage is ./server <port>\n");
        exit(1);
    }
    
    bzero((char *)&socket_addr, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = INADDR_ANY;
    socket_addr.sin_port = htons(listening_socket_port);
    
    if ((listening_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: Listening socket");
        exit(1);
    }
    
    if ((bind(listening_socket, (struct sockaddr *)&socket_addr, sizeof(socket_addr))) < 0) {
        perror("Error: Binding socket");
        exit(1);
    }
    
    listen(listening_socket, MAX_SESSIONS);
    
    
    FD_ZERO(&allSocketsSet);
    FD_SET(listening_socket, &allSocketsSet);
    highestSocketID = listening_socket;
    
    while (1) {

        socketsToRead = allSocketsSet;        
        select(highestSocketID +1, &socketsToRead, NULL, NULL, NULL);
        int len = sizeof(socket_addr);
        
        if (FD_ISSET(listening_socket, &socketsToRead)) {
            int client_socket = accept(listening_socket, (struct sockaddr *)&socket_addr, (socklen_t *)&len);
            if (client_socket < 0) {
                printf("Socket não pode ser aberto para cliente\n");
            } else {
                for (i = 0; i < MAX_SESSIONS && (client_sessions[i].socket_id != -1); i++);
                if (i == MAX_SESSIONS) {
                    printf("Usuário negado devido ao limite\n");
                    close(client_socket);
                } else {
                    client_sessions[i].socket_id = client_socket;
                    client_sessions[i].person = NULL;
                    
                    FD_SET(client_socket, &allSocketsSet);
                    if (client_socket > highestSocketID)
                        highestSocketID = client_socket;
                }
            }
        }
        
        for (i = 0; i < MAX_SESSIONS; i++) {
            if (client_sessions[i].socket_id > 0 && FD_ISSET(client_sessions[i].socket_id, &socketsToRead)) {
                session *currentSession = client_sessions+i;
                readCommand(currentSession);
            }
        }
        
        for (i = 0; i < MAX_SESSIONS; i++) {
            if (client_sessions[i].socket_id > 0) {
                updateMessages(&client_sessions[i]);
            }
        }
        
    }
    
    
    
    return 0;
}

void readCommand(session *currentSession) {
    
    char str[MAX_LINE];
    char buf[MAX_LINE];
    char *aux;
    message *msg;
    memset(str, '\0', MAX_LINE);
    memset(buf, '\0', MAX_LINE);
    recv(currentSession->socket_id, str, sizeof(str), 0);
    messageType type = (messageType) atoi(getNWord(str,1));
    
    printf("recebido no server...%s...\n", str);
    
    if (type == login_msg) {
        currentSession->person = getPersonByName(getNWord(str,2));
        currentSession->person->online = 1;
        printf("User %s joined the chat.\n", currentSession->person->name);
        
    } else if (type == new_msg) {
        msg = calloc(1, sizeof(message));
        msg->text = getNWord(str, 0);
        msg->id = getNWord(str,2);
        msg->receiver = getPersonByName(getNWord(str,3));
        msg->sender = currentSession->person;
        msg->type = new_msg;
        msg->group = NULL;
		if (sendMessage(msg)) {
		    msg = calloc(1, sizeof(message));
		    msg->text = "\0";
		    msg->id = getNWord(str,2);
		    msg->receiver = currentSession->person;
		    msg->sender = currentSession->person;
		    msg->type = sent_msg;
		    msg->group = NULL;
			sendMessage(msg);
		}
		
    } else if (type == recv_msg) {
		msg = calloc(1, sizeof(message));
		msg->id = getNWord(str,2);
		msg->receiver = getPersonByName(getNWord(str,3));
		msg->sender = currentSession->person;
		msg->type = recv_msg;
        msg->group = NULL;
		sendMessage(msg);
	
    } else if (type == recvgrp_msg) {
        msg = calloc(1, sizeof(message));
        msg->id = getNWord(str,2);
        msg->receiver = getPersonByName(getNWord(str,3));
        msg->sender = currentSession->person;
        msg->type = recvgrp_msg;
        msg->group = (struct Group*)getGroupByName(getNWord(str, 4));
        decreaseCounter(msg);
        
    } else if (type == newgrp_msg) {
        Group *group = getGroupByName(getNWord(str, 3));
        
        msg = calloc(1, sizeof(message));
        msg->type = newgrp_msg;
        msg->sender = currentSession->person;
        msg->group = (struct Group*)group;
        msg->id = getNWord(str, 2);
        msg->text = getNWord(str, 0);
        
        if (sendMessage(msg)) {
            msg = calloc(1, sizeof(message));
            msg->type = sent_msg;
            msg->receiver = currentSession->person;
            msg->id = getNWord(str, 2);
            sendMessage(msg);
        } else {
            msg = calloc(1, sizeof(message));
            msg->type = error_msg;
            msg->text = "Não foi possível enviar mensagem!";
            msg->receiver = currentSession->person;
            sendMessage(msg);
            return;
        }
    } else if (type == join_group) {
        Group *group = getGroupByName(getNWord(str, 2));
        if (addUserToGroup(group, currentSession->person)) {
            msg = calloc(1, sizeof(message));
            msg->type = join_group;
            msg->sender = currentSession->person;
            msg->group = (struct Group*)group;
            
            sendMessage(msg);
        } else {
            msg = calloc(1, sizeof(message));
            msg->type = error_msg;
            msg->text = "Você já está no grupo!";
            msg->receiver = currentSession->person;
            sendMessage(msg);
        }
        
    } else if (type == create_group) {
        char * groupName = getNWord(str, 2);
        Group *group = getGroupByName(groupName);
        msg = calloc(1, sizeof(message));
        msg->type = create_group;
        msg->text = groupName;
        msg->group = (struct Group*)group;
        msg->receiver = currentSession->person;
        msg->sender = currentSession->person;
        addUserToGroup(group, currentSession->person);
        sendMessage(msg);
        
    } else if (type == invite_player) {
        msg = calloc(1, sizeof(message));
        msg->type = invite_player;
        msg->receiver = getPersonByName(getNWord(str,3));
        msg->sender = currentSession->person;
        msg->id = getNWord(str, 2);
        sendMessage(msg);
        
        msg = calloc(1, sizeof(message));
        msg->type = invite_sent;
        msg->receiver = currentSession->person;
        msg->id = getNWord(str,2);
        sendMessage(msg);
    } else if (type == accept_invite) {
        
        createGame(getPersonByName(getNWord(str,2)), currentSession->person);
        
        msg = calloc(1, sizeof(message));
        msg->type = accept_invite;
        msg->receiver = getPersonByName(getNWord(str,2));
        msg->sender = currentSession->person;
        sendMessage(msg);
    } else if (type == refuse_invite) {
        msg = calloc(1, sizeof(message));
        msg->type = refuse_invite;
        msg->receiver = getPersonByName(getNWord(str,2));
        msg->sender = currentSession->person;
        sendMessage(msg);
    } else if (type == game_move) {
        
        int mv[2];
        mv[0] = ((char*)getNWord(str, 4))[0]-'A';
        mv[1] = atoi(getNWord(str, 5)) - 1;
        
        person *playerA = currentSession->person;
        person *playerB = getPersonByName(getNWord(str, 3));
        
        game *g = getGame(playerA, playerB);
        if (g == NULL) {
            msg = calloc(1, sizeof(message));
            msg->type = error_msg;
            msg->text = "Jogo inexistente!";
            msg->receiver = currentSession->person;
            sendMessage(msg);
            return;
        }
        if (makeMove(g, mv, currentSession->person) == 0) {
            msg = calloc(1, sizeof(message));
            msg->type = error_msg;
            msg->text = "Jogada inválida!";
            msg->receiver = currentSession->person;
            sendMessage(msg);
            return;
        }
        
        msg = calloc(1, sizeof(message));
        msg->type = move_sent;
        msg->id = getNWord(str,2);
        msg->receiver = currentSession->person;
        sendMessage(msg);

        msg = calloc(1, sizeof(message));
        msg->type = game_move;
        msg->id = getNWord(str,2);
        msg->sender = currentSession->person;
        msg->receiver = getPersonByName(getNWord(str,3));
        sendMessage(msg);
        
        if (getWinner(g) == X) {
            msg = calloc(1, sizeof(message));
            msg->type = won_game;
            msg->sender = g->playerB;
            msg->receiver = g->playerA;
            sendMessage(msg);
            
            msg = calloc(1, sizeof(message));
            msg->type = lost_game;
            msg->sender = g->playerA;
            msg->receiver = g->playerB;
            sendMessage(msg);
        } else if (getWinner(g) == O){
            msg = calloc(1, sizeof(message));
            msg->type = won_game;
            msg->sender = g->playerA;
            msg->receiver = g->playerB;
            sendMessage(msg);
                
            msg = calloc(1, sizeof(message));
            msg->type = lost_game;
            msg->sender = g->playerB;
            msg->receiver = g->playerA;
            sendMessage(msg);

        } else if (getWinner(g) == D)   {
            msg = calloc(1, sizeof(message));
            msg->type = draw_game;
            msg->sender = g->playerB;
            msg->receiver = g->playerA;
            sendMessage(msg);

            msg = calloc(1, sizeof(message));
            msg->type = draw_game;
            msg->sender = g->playerA;
            msg->receiver = g->playerB;
            sendMessage(msg);

        }
    } else if (type == move_received) {
        msg = calloc(1, sizeof(message));
        msg->type = move_received;
        msg->id = getNWord(str, 2);
        msg->receiver = getPersonByName(getNWord(str,3));
        sendMessage(msg);
    } else if (type == invite_received) {
        msg = calloc(1, sizeof(message));
        msg->type = invite_received;
        msg->receiver = getPersonByName(getNWord(str,3));
        msg->id = getNWord(str, 2);
        sendMessage(msg);
    } else if (type == who_msg) {
		msg = calloc(1, sizeof(message));
		msg->text = SetWhoMessage();
		msg->receiver = currentSession->person;
		msg->type = who_msg;
		sendMessage(msg);

    } else if (type == exit_msg) {
        currentSession->socket_id = -1;
        currentSession->person->online = 0;

    } else if (type == error_msg) {
    	printf("error... bad message\n");
		msg = calloc(1, sizeof(message));
		msg->text = "\0";
		msg->receiver = currentSession->person;
		msg->id = "\0";
		msg->sender = currentSession->person;
		msg->type = error_msg;
        msg->group = NULL;
		sendMessage(msg);

    }
    
}

char* SetWhoMessage () {

    	int i, size = 0;
	    int numberOfUsers;
	    person **allUsers;
		char buf[MAX_LINE];
		char aux[30];

		memset(buf, '\0', MAX_LINE);
        allUsers = getAllUsers(&numberOfUsers);
        snprintf (buf, MAX_LINE, "\"%d ", numberOfUsers);
        
        for (i = 0; i < numberOfUsers; i++) {
            if (allUsers[i] != NULL && allUsers[i]->online) {
                strcat(buf, allUsers[i]->name);
                strcat(buf, " ");
                if (strlen(allUsers[i]->name) > size)
                	size = strlen(allUsers[i]->name);
            }
        }
        
        strcat(buf, "| ");
        
        for (i = 0; i < numberOfUsers; i++) {
            if (allUsers[i] != NULL && allUsers[i]->online == 0) {
                strcat(buf, allUsers[i]->name);
                strcat(buf, " ");
                if (strlen(allUsers[i]->name) > size)
                	size = strlen(allUsers[i]->name);
            }
        }
        
        sprintf(aux, "%d\"", size);
        strcat(buf,aux);
        return getNWord(buf,0);

}

void updateMessages (session *clientSession) {
    
    int i, j;
    char buffer[MAX_LINE];
    
    if (clientSession->socket_id <= 0)
        return;
    
    if (clientSession->person == NULL)
        return;
    
    for (i = 0; i < MAX_MESSAGE_QUEUE; i++) {
        message *msg = (message*)clientSession->person->queue[clientSession->person->begin];
        memset(buffer, '\0', MAX_LINE);
		if (msg != NULL) {
            
            messageType type = msg->type;
            if (type == new_msg) {
                sprintf(buffer, "%d %s %s \"%s\"", msg->type, msg->id, msg->sender->name, msg->text);
            } else if (type == recv_msg) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == recvgrp_msg) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == newgrp_msg) {
                sprintf(buffer, "%d %s %s %s \"%s\"", msg->type, msg->id, msg->sender->name, ((Group*)msg->group)->name ,msg->text);
            } else if (type == sent_msg) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == join_group) {
                sprintf(buffer, "%d %s %s", msg->type,((Group*)msg->group)->name, msg->sender->name);
            } else if (type == create_group) {
                sprintf(buffer, "%d %s", msg->type,((Group*)msg->group)->name);
            } else if (type == invite_player) {
                sprintf(buffer, "%d %s %s", msg->type, msg->id ,msg->sender->name);
            } else if (type == accept_invite) {
                sprintf(buffer, "%d %s", msg->type, msg->sender->name);
            } else if (type == refuse_invite) {
                sprintf(buffer, "%d %s", msg->type, msg->sender->name);
            } else if (type == move_sent) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == game_move) {
                game *g = getGame(msg->sender, msg->receiver);
                char state[10];
                for (int i = 0; i < 9; i++) {
                    switch (g->board[i%3][i/3]) {
                        case X:
                            state[i] = 'X';
                            break;
                        case O:
                            state[i] = 'O';
                            break;
                        default:
                            state[i] = ' ';
                            break;
                    }
                }
                state[9] = '\0';
                sprintf(buffer, "%d %s %s \"%s\"", msg->type, msg->id, msg->sender->name, state);
                
            } else if (type == move_received) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == invite_sent) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == invite_received) {
                sprintf(buffer, "%d %s", msg->type, msg->id);
            } else if (type == who_msg) {
                sprintf(buffer, "%d \"%s\"", msg->type, msg->text);
            } else if (type == error_msg) {
                sprintf(buffer, "%d \"%s\"", msg->type, msg->text);
            } else if (type == won_game) {
                sprintf(buffer, "%d %s", msg->type, msg->sender->name);
            } else if (type == lost_game) {
                sprintf(buffer, "%d %s", msg->type, msg->sender->name);
            } else if (type == draw_game) {
                sprintf(buffer, "%d %s", msg->type, msg->sender->name);
            }
            
            if (send(clientSession->socket_id, buffer, strlen(buffer)+1, 0) > 0) {
                clientSession->person->queue[i] = NULL;
                clientSession->person->begin = (clientSession->person->begin + 1) % MAX_MESSAGE_QUEUE;
            }
            
        }
        
    }
    
}

