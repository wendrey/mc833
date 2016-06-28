//
//  database.c
//  ChatServer
//
//  Created by Lucas Calzolari on 6/23/16.
//  Copyright © 2016 grupomodafoca. All rights reserved.
//

#include "database.h"
#include "string.h"
#include <stdlib.h>

#define USER_HASH_TABLE_SIZE 10000
#define MAX_USER 100
#define MAX_GROUPS 20

person *users[MAX_USER];
Group *groups[MAX_GROUPS];

unsigned int sendMessageToGroup (message *msg);
unsigned int sendMessageToPerson (message *msg);

void setupDatabase () {
    int i;
    for (i = 0; i < MAX_USER; i++) {
        users[i] = NULL;
    }
    for (i = 0; i < MAX_GROUPS; i++) {
        groups[i] = NULL;
    }
}

unsigned int hash (char *string) {
    int hash = 5381;
    int c;
    
    while ((c = *string++))
        hash = ((hash << 5) + hash) + c;
    
    return (unsigned int) hash % USER_HASH_TABLE_SIZE;
}

person *getPersonByName (char *name){
    
    int i;
    int pos = 0;
    
    while (users[pos] != NULL && strcmp(name, users[pos]->name)) {
        pos++;
    }
    
    if (users[pos] == NULL) {
        users[pos] = malloc(sizeof(person));
        strcpy(users[pos]->name, name);
        for (i = 0; i < MAX_MESSAGE_QUEUE; i++) {
            users[pos]->queue[i] = NULL;
        }
    }
    
    return users[pos];
};

person **getAllUsers (int *size) {
    *size = MAX_USER;
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
    }
    
    return groups[pos];
};

unsigned int sendMessage (message *msg) {
    
    if (msg->group == NULL) {
        return sendMessageToPerson(msg);
    } else {
        return sendMessageToGroup(msg);
    }
}

unsigned int sendMessageToGroup (message *msg) {
    Group *group = (Group*)msg->group;
    int i, id = 0;
    for (i = 0; i < MAX_PERSON_PER_GROUP; i++) {
        if (group->members[i] != NULL) {
            message *copyMessage = malloc(sizeof(message));
            copyMessage = msg;
            id = sendMessageToPerson(copyMessage) > id ? : id;
        }
    }
    free(msg);
    return id;
}

unsigned int sendMessageToPerson (message *msg) {
    int i;
    person *receiver = msg->receiver;
    for (i = 0; i < MAX_MESSAGE_QUEUE ; i++) {
        if (receiver->queue[i] == NULL) {
            receiver->queue[i] = (struct message *)msg;
            return (unsigned int)hash(msg->text);
        }
    }
    return 0;
}

