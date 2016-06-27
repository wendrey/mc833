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

person *users[MAX_USER];

int hash (char *string) {
    int hash = 5381;
    int c;
    
    while ((c = *string++))
        hash = ((hash << 5) + hash) + c;
    
    return hash % USER_HASH_TABLE_SIZE;
}

person *getPersonByName (char *name){
    
    int pos = 0;
    
    while (pos < MAX_USER && users[pos] != NULL && strcmp(name, users[pos]->name)) {
        pos++;
    }
    
    if (pos < MAX_USER && users[pos] == NULL) {
        users[pos] = malloc(sizeof(person));
        strcpy(users[pos]->name, name);
        for (int i = 0; i < MAX_MESSAGE_QUEUE; i++) {
            users[pos]->queue[i] = NULL;
        }
    }
    
    return users[pos];
};

person **getAllUsers (int *size) {
    *size = MAX_USER;
    return users;
}

unsigned int sendMessage (message *msg) {
    person *receiver = msg->receiver;
    for (int i = 0; i < MAX_MESSAGE_QUEUE ; i++) {
        if (receiver->queue[i] == NULL) {
            receiver->queue[i] = (struct message *)msg;
            return (unsigned int)hash(msg->text);
        }
    }
    return 0;
}
