
#include "database.h"

typedef struct {
    int missing;
    message *msg;
} statusCounter;

game *games[MAX_GAMES];

int signed_users = 0;
person *users[MAX_USER];
Group *groups[MAX_GROUPS];
statusCounter counters[MAX_COUNTER];

void setupDatabase () {
    int i;
    for (i = 0; i < MAX_USER; i++) {
        users[i] = NULL;
    }
    
    for (i = 0; i < MAX_GROUPS; i++) {
        groups[i] = NULL;
    }
    
    for (i = 0; i < MAX_COUNTER; i++) {
        counters[i].msg = NULL;
    }
    
    for (i = 0; i < MAX_GAMES; i++) {
        games[i]= NULL;
    }
}

message *duplicateMessage (message *originalMessage) {
    message *newMessage = calloc(1, sizeof(message));
    newMessage->group = originalMessage->group;
    newMessage->receiver = originalMessage->receiver;
    newMessage->sender = originalMessage->sender;
    newMessage->type = originalMessage->type;
    
    if (originalMessage->text) {
        newMessage->text = calloc((strlen(originalMessage->text) + 1), sizeof(char));
        strcpy(newMessage->text, originalMessage->text);
    }
    
    if (originalMessage->id) {
        newMessage->id = calloc(1, (strlen(originalMessage->id) + 1));
        strcpy(newMessage->id, originalMessage->id);
    }
    
    return  newMessage;
}

person *getPersonByName (char *name){
    
    int i;
    int pos = 0;
    
    while (users[pos] != NULL && strcmp(name, users[pos]->name)) {
        pos++;
    }
    
    if (users[pos] == NULL) {
        signed_users++;
        users[pos] = malloc(sizeof(person));
        strcpy(users[pos]->name, name);
        for (i = 0; i < MAX_MESSAGE_QUEUE; i++) {
            users[pos]->queue[i] = NULL;
        }
        users[pos]->begin = 0;
        users[pos]->end = 0;
    }
    
    return users[pos];
};

person **getAllUsers (int *size) {
    *size = signed_users;
    return users;
}

Group *getGroupByName (char *name){
    
    int i;
    int pos = 0;
    
    while (groups[pos] != NULL && strcmp(name, groups[pos]->name)) {
        pos++;
    }
    
    if (groups[pos] == NULL) {
        groups[pos] = malloc(sizeof(Group));
        strcpy(groups[pos]->name, name);
        for (i = 0; i < MAX_PERSON_PER_GROUP; i++) {
            groups[pos]->members[i] = NULL;
        }
        groups[pos]->numberOfMembers = 0;
    }
    
    return groups[pos];
};

void setCounter(message *m) {
    int i;
    
    if (!m || !m->group || m->type != newgrp_msg)
        return;
    
    for (i = 0; counters[i].msg && i < MAX_COUNTER; i++);
    
    if (i == MAX_COUNTER )
        return;
    
    counters[i].msg = duplicateMessage(m);
    counters[i].missing = ((Group*)m->group)->numberOfMembers - 1;
}

int sendMessage (message *msg) {
    
    setCounter(msg);
    if (msg->group == NULL) {
        return sendMessageToPerson(msg);
    } else {
        return sendMessageToGroup(msg);
    }
}

int isUserInGroup (person *user, Group *group) {
    for (int i = 0; i < MAX_PERSON_PER_GROUP; i++) {
        if (group->members[i] == user)
            return 1;
    }
    return 0;
}

int sendMessageToGroup (message *msg) {
    Group *group = (Group*)msg->group;
    if (!isUserInGroup(msg->sender, group)) {
        return 0;
    }
    int i, n;
    n = group->numberOfMembers - 1;
    for (i = 0; i < MAX_PERSON_PER_GROUP; i++) {
        if (group->members[i] != NULL) {
            if (msg->type == newgrp_msg && group->members[i] == msg->sender) {
                continue;
            }
            message *copyMessage = duplicateMessage(msg);
            copyMessage->receiver = group->members[i];
            if (sendMessageToPerson(copyMessage))
                n--;
        }
    }
    free(msg);
    return !n;
}

int sendMessageToPerson (message *msg) {
    int i, j;
    person *receiver = msg->receiver;
    for (i = 0; i < MAX_MESSAGE_QUEUE ; i++) {
        j = (receiver->end + i) % MAX_MESSAGE_QUEUE;
        if (receiver->queue[j] == NULL) {
            receiver->queue[j] = (struct message *)msg;
            receiver->end = (j+1) % MAX_MESSAGE_QUEUE;
            return 1;
        }
    }
    return 0;
}

int addUserToGroup (Group *g, person *p) {
    if (isUserInGroup(p, g)) {
        return 0;
    }
    for (int i = 0; i < MAX_PERSON_PER_GROUP; i++) {
        if (g->members[i] == NULL) {
            g->members[i] = p;
            g->numberOfMembers++;
            return 1;
        }
    }
    return 0;
}

int sameMessage (message *A, message *B) {
    if (!A || !B)
        return 0;
    if (strcmp(A->id, B->id))
        return  0;
    if (A->group != B-> group)
        return 0;
    return 1;
}

void decreaseCounter(message *m) {
    for (int i = 0; i < MAX_COUNTER; i++) {
        if (sameMessage(counters[i].msg, m)) {
            counters[i].missing--;
            if (counters[i].missing == 0) {

                message *ackMsg = counters[i].msg;
                ackMsg->group = NULL;
                ackMsg->type = recv_msg;
                ackMsg->receiver = ackMsg->sender;
                
                sendMessage(ackMsg);
                
                counters[i].msg = NULL;
            }
        }
    }
}

int samePlayers (person *A, person *B, game *g) {
    if (g && g->playerA == A && g->playerB == B)
        return 1;
    if (g && g->playerA == B && g->playerB == A)
        return 1;
    return 0;
}

game *getGame (person *A, person *B) {
    int i;
    for (i = 0; i < MAX_GAMES; i++) {
        if (samePlayers(A, B, games[i]))
            return games[i];
    }
    
    return NULL;
}

int createGame (person *A, person *B) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] == NULL) {
            games[i] = calloc(1, sizeof(game));
            games[i]->playerA = A;
            games[i]->playerB = B;
            games[i]->turn = X;
            return 1;
        }
    }
    
    return 0;
}

boardState getWinner (game *g) {
    
    if (g->board[0][0] == X && g->board[0][1] == X && g->board[0][2] == X) {
        return X;
    }
    if (g->board[1][0] == X && g->board[1][1] == X && g->board[1][2] == X) {
        return X;
    }
    if (g->board[2][0] == X && g->board[2][1] == X && g->board[2][2] == X) {
        return X;
    }
    if (g->board[0][0] == X && g->board[1][0] == X && g->board[2][0] == X) {
        return X;
    }
    if (g->board[0][1] == X && g->board[1][1] == X && g->board[2][1] == X) {
        return X;
    }
    if (g->board[0][2] == X && g->board[1][2] == X && g->board[2][2] == X) {
        return X;
    }
    if (g->board[0][0] == X && g->board[1][1] == X && g->board[2][2] == X) {
        return X;
    }
    if (g->board[0][2] == X && g->board[1][1] == X && g->board[2][0] == X) {
        return X;
    }
    
    
    if (g->board[0][0] == O && g->board[0][1] == O && g->board[0][2] == O) {
        return O;
    }
    if (g->board[1][0] == O && g->board[1][1] == O && g->board[1][2] == O) {
        return O;
    }
    if (g->board[2][0] == O && g->board[2][1] == O && g->board[2][2] == O) {
        return O;
    }
    if (g->board[0][0] == O && g->board[1][0] == O && g->board[2][0] == O) {
        return O;
    }
    if (g->board[0][1] == O && g->board[1][1] == O && g->board[2][1] == O) {
        return O;
    }
    if (g->board[0][2] == O && g->board[1][2] == O && g->board[2][2] == O) {
        return O;
    }
    if (g->board[0][0] == O && g->board[1][1] == O && g->board[2][2] == O) {
        return O;
    }
    if (g->board[0][2] == O && g->board[1][1] == O && g->board[2][0] == O) {
        return O;
    }
    
    int counter = 0;
    for (int i = 0; i <= 2; i++) {
        for (int j = 0; j <= 2; j++)
            if (g->board[i][j] != B) {
                counter++;
            }
    }
    if (counter == 9) {
        return D;
    }
    return B;
    
};

int makeMove (game *g, int mv[2], person *player) {
    
    if (g->board[mv[0]][mv[1]] != B || mv[0] > 2 || mv[1] > 2 || mv[0] < 0 || mv[1] < 0) {
        return 0;
    }
    
    if ((g->turn == X && player == g->playerB) || (g->turn == O && player == g->playerA)) {
        return 0;
    }
    
    g->board[mv[0]][mv[1]] = g->turn;
    g->turn = g->turn == X ? O :  X;
    
    return 1;
    
}
void removeGame (person *playerA, person *playerB) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] && games[i]->playerA == playerA && games[i]->playerB == playerB) {
            games[i] = NULL;
        }
    }
}

