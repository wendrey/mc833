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
#define MAX_PERSON_PER_GROUP 10

struct message;
struct Group;

typedef struct {
    char name[30];
    int online;
    struct message *queue[MAX_MESSAGE_QUEUE];
} person;

typedef struct {
    char *text;
    person *sender;
    person *receiver;
    struct Group *group;
} message;

typedef struct {
    char name[30];
    person *members[MAX_PERSON_PER_GROUP];
} Group;

person *getPersonByName (char *name);
person **getAllUsers (int *size);
unsigned int sendMessage (message *msg);
void setupDatabase ();