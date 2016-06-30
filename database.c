
#include "database.h"

typedef struct {
    int missing;
    message *msg;
} statusCounter;

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

int sendMessageToGroup (message *msg) {
    Group *group = (Group*)msg->group;
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


