
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
                printf("Socket não pode ser aberto para cliente");
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
    recv(currentSession->socket_id, &str, sizeof(str), 0);
    messageType type = (messageType) atoi(getNWord(str,1));
    
    printf("recebido no server...%s...\n", str);
    
    if (type == login_msg) {
        currentSession->person = getPersonByName(getNWord(str,2));
        currentSession->person->online = 1;
        currentSession->person->begin = 0;
        currentSession->person->end = 0;
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
		msg->text = "\0";
		msg->id = getNWord(str,2);
		msg->receiver = getPersonByName(getNWord(str,3));
		msg->sender = currentSession->person;
		msg->type = recv_msg;
        msg->group = NULL;
		sendMessage(msg);
	
    } else if (type == newgrp_msg) {

//	} else if (type == recvgrp_msg) {

    } else if (type == join_msg) {

    } else if (type == create_msg) {

    } else if (type == game_msg) {

    } else if (type == play_msg) {

    } else if (type == who_msg) {
		msg = calloc(1, sizeof(message));
		msg->text = SetWhoMessage();
		msg->id = "\0";
		msg->receiver = currentSession->person;
		msg->sender = currentSession->person;
		msg->type = who_msg;
        msg->group = NULL;
		sendMessage(msg);

    } else if (type == exit_msg) {
        close(currentSession->socket_id);
        currentSession->socket_id = -1;
        currentSession->person->online = 0;
        printf("User %s left the chat.\n", currentSession->person->name);

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

void executeCommand (command *currentCommand, session *currentSession) {

    int i;
    int numberOfUsers;
    person **allUsers;
    char buffer[MAX_LINE];
    memset (buffer, '\0', MAX_LINE);
    
    switch (currentCommand->type) {
        case loginUser:
            /*currentSession->person = getPersonByName((char*)(currentCommand->data));
            currentSession->person->online = 1;
            printf("User %s joined the chat.\n", currentSession->person->name);
            */break;
        case sendUserMessage:
            /*msg = calloc(1, sizeof(message));
            msg->sender = currentSession->person;
            msg->receiver = getPersonByName(currentCommand->receiver);
            msg->text = (char*)currentCommand->data;
		    sprintf(buffer, "%u", sendMessage(msg));
            */break;
        case createGroup:
            break;
        case joinGroup:
            break;
        case sendGroupMesage:
            break;
        case checkUsers:
            /*allUsers = getAllUsers(&numberOfUsers);
            for (i = 0; i < numberOfUsers; i++) {
                if (allUsers[i] != NULL && allUsers[i]->online) {
                    strcat(buffer, allUsers[i]->name);
                    strcat(buffer, " ");
                }
            }
            strcat(buffer, "| ");
            for (i = 0; i < numberOfUsers; i++) {
                if (allUsers[i] != NULL && allUsers[i]->online == 0) {
                    strcat(buffer, allUsers[i]->name);
                    strcat(buffer, " ");
                }
            }
    	    send(currentSession->socket_id, buffer, strlen(buffer)+1, 0);	
            */break;
        case logoff:
            /*close(currentSession->socket_id);
            currentSession->socket_id = -1;
            currentSession->person->online = 0;
            printf("User %s left the chat.\n", currentSession->person->name);
            */break;
        case game:
            break;
        case sendFile:
            break;
        case invalid:
            /*sprintf (buffer, "That's an invalid command, sorry!\n");
    		send(currentSession->socket_id, buffer, strlen(buffer)+1, 0);
        	*/break;
        default:
            break;
    }
    
    
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
            if (msg->group == NULL) {
                sprintf(buffer, "%d %s %s \"%s\"", msg->type, msg->id, msg->sender->name, msg->text);
                printf("%s\n", buffer);
            } else { return;
                sprintf(buffer, "GROUPMESSAGE %s %s \"%s\"", ((Group*)msg->group)->name, msg->sender->name, msg->text);
            }
            if (send(clientSession->socket_id, buffer, strlen(buffer), 0) > 0) {
            	clientSession->person->queue[i] = NULL;
            	clientSession->person->begin = (clientSession->person->begin + 1) % MAX_MESSAGE_QUEUE; 
            }
        }
    }
    
}

