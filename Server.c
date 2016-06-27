//
//  main.c
//  ChatServer
//
//  Created by Lucas Calzolari on 6/23/16.
//  Copyright © 2016 grupomodafoca. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <Utils.h>
#include <database.h>

#define MAX_SESSIONS 10

typedef struct {
    int socket_id;
    person *person;
} session;

typedef enum { loginUser =0, sendUserMessage, createGroup, joinGroup, sendGroupMesage, checkUsers, sendFile, game, logoff, invalid
} commandType;

typedef struct {
    commandType type;
    char *receiver;
    void *data;
} command;

session client_sessions[MAX_SESSIONS];

command *readCommand(session *currentSession);
void executeCommand (command *currentCommand, session *currentSession);
void updateMessages (session *clientSession);

int main(int argc, const char * argv[]) {
    
    struct sockaddr_in socket_addr;
    int listening_socket;
    int listening_socket_port;

    for (int i = 0; i < MAX_SESSIONS; i++) {
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
        perror("Error: Listening socket creation");
        exit(1);
    }
    
    bzero((char *)&socket_addr, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = INADDR_ANY;
    socket_addr.sin_port = htons(listening_socket_port);
    
    if ((listening_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: Listening socket creation");
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
                int i;
                for (int i = 0; i < MAX_SESSIONS || (client_sessions[i].socket_id == -1); i++);
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
        
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (client_sessions[i].socket_id > 0 && FD_ISSET(client_sessions[i].socket_id, &socketsToRead)) {
                session *currentSession = client_sessions+i;
                
                command *receivedCommand = readCommand(currentSession);
                executeCommand(receivedCommand, currentSession);
            }
        }
        
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (client_sessions[i].socket_id > 0) {
                updateMessages(&client_sessions[i]);
            }
        }
        
    }
    
    
    
    return 0;
}

command *readCommand(session *currentSession) {
    
    char buffer[256];
    
    recv(currentSession->socket_id, &buffer, sizeof(buffer), 0);
    
    char *commandWord = getNWord(buffer, 1);
    
    if (commandWord == NULL) {
        return NULL;
    }
    upperCaseString(commandWord);
    
    command *currentCommand = calloc(1, sizeof(command));
    
    if (strcmp(commandWord, "LOGIN") == 0) {
        char *name = getNWord(buffer, 2);
        currentCommand->type = loginUser;
        currentCommand->data = name;
    } else if (strcmp(commandWord, "SEND") ==   0) {
        currentCommand->type = sendUserMessage;
        currentCommand->receiver = getNWord(buffer, 2);
        currentCommand->data = getNWord(buffer, 3);
    } else if (strcmp(commandWord, "CREATEG") == 0) {
        currentCommand->type = createGroup;
    } else if (strcmp(commandWord, "JOING") == 0) {
        currentCommand->type = joinGroup;
    } else if (strcmp(commandWord, "SENDG") == 0) {
        currentCommand->type = sendGroupMesage;
    } else if (strcmp(commandWord, "WHO") == 0) {
        currentCommand->type = checkUsers;
    } else if (strcmp(commandWord, "EXIT") == 0) {
        currentCommand->type = logoff;
    } else if (strcmp(commandWord, "GAME") == 0) {
        currentCommand->type = game;
    } else if (strcmp(commandWord, "SENDF") == 0) {
        currentCommand->type = sendFile;
    } else {
        currentCommand->type = invalid;
    }
    
    free(commandWord);
    
    return currentCommand;
}



void executeCommand (command *currentCommand, session *currentSession) {
    
    int numberOfUsers;
    person **allUsers;
    char buffer[256];
    message *msg;
    buffer[0] = '\0';
    
    switch (currentCommand->type) {
        case loginUser:
            currentSession->person = getPersonByName((char*)(currentCommand->data));
            currentSession->person->online = 1;
            printf("User %s joined the chat.\n", currentSession->person->name);
            break;
        case sendUserMessage:
            msg = malloc(sizeof(message));
            msg->sender = currentSession->person;
            msg->receiver = getPersonByName(currentCommand->receiver);
            msg->text = (char*)currentCommand->data;
            sprintf(buffer, "%u", sendMessage(msg));
            break;
        case createGroup:
            break;
        case joinGroup:
            break;
        case sendGroupMesage:
            break;
        case checkUsers:
            allUsers = getAllUsers(&numberOfUsers);
            for (int i = 0; i < numberOfUsers; i++) {
                if (allUsers[i] != NULL && allUsers[i]->online) {
                    strcat(buffer, allUsers[i]->name);
                    strcat(buffer, " ");
                }
            }
            strcat(buffer, "| ");
            for (int i = 0; i < numberOfUsers; i++) {
                if (allUsers[i] != NULL && allUsers[i]->online == 0) {
                    strcat(buffer, allUsers[i]->name);
                    strcat(buffer, " ");
                }
            }
            break;
        case logoff:
            close(currentSession->socket_id);
            currentSession->socket_id = -1;
            currentSession->person->online = 0;
            printf("User %s left the chat.\n", currentSession->person->name);
            break;
        case game:
            break;
        case sendFile:
            break;
        case invalid:
            sprintf (buffer, "INVALID COMMAND\n");
        default:
            break;
    }
    
    send(currentSession->socket_id, buffer, strlen(buffer), 0);
    
}

void updateMessages (session *clientSession) {
    
    char buffer[256];
    
    if (clientSession->socket_id <= 0)
        return;
    
    if (clientSession->person == NULL) {
        return;
    }
    
    for (int i = 0; i < MAX_MESSAGE_QUEUE; i++) {
        message *msg = (message*)clientSession->person->queue[i];
        if (msg != NULL) {
            if (msg->group == NULL) {
                sprintf(buffer, "MESSAGE %s %s", msg->sender->name, msg->text);
            } else {
                sprintf(buffer, "GROUPMESSAGE %s %s %s", ((Group*)msg->group)->name, msg->sender->name, msg->text);
            }
            send(clientSession->socket_id, buffer, strlen(buffer), 0);
        }
    }
    
}

