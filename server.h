
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "utils.h"

#define MAX_LINE 256
#define MAX_SESSIONS 10

typedef struct {
    int socket_id;
    person *person;
} session;

char* SetWhoMessage();
void readCommand(session *currentSession);
void updateMessages (session *clientSession);
int GetMessageType (char *string);

