//
//  database.h
//  ChatServer
//
//  Created by Lucas Calzolari on 6/23/16.
//  Copyright © 2016 grupomodafoca. All rights reserved.
//

#include <stdio.h>

#define MAX_MESSAGE_SIZE 256
#define MAX_MESSAGE_QUEUE 20

typedef struct {
    char name[30];
    int online;
    struct message *queue[MAX_MESSAGE_QUEUE];
} person;

typedef struct {
    char *text;
    person *sender;
    person *receiver;
} message;

person *getPersonByName (char *name);
person **getAllUsers (int *size);
unsigned int sendMessage (message *msg);
