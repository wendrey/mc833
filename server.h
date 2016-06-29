
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "utils.h"
#include "database.h"

#define MAX_LINE 256
#define MAX_SESSIONS 10

typedef struct {
    int socket_id;
    person *person;
} session;

typedef enum {
    loginUser = 0, sendUserMessage, createGroup, joinGroup,
    sendGroupMesage, checkUsers, sendFile, game, logoff, invalid
} commandType;

typedef struct {
    commandType type;
    char *receiver;
    void *data;
} command;

char* SetWhoMessage();
void readCommand(session *currentSession);
void executeCommand (command *currentCommand, session *currentSession);
void updateMessages (session *clientSession);

