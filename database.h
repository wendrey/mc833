
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USER_HASH_TABLE_SIZE 10000
#define MAX_USER 100
#define MAX_GROUPS 20
#define MAX_MESSAGE_SIZE 256
#define MAX_MESSAGE_QUEUE 20
#define MAX_PERSON_PER_GROUP 10
#define MAX_COUNTER 50

struct message;
struct Group;

typedef struct {
    char name[30];
    int online;
    int begin;
    int end;
    struct message *queue[MAX_MESSAGE_QUEUE];
} person;

typedef enum { new_msg = 0, sent_msg, recv_msg,
			   newgrp_msg, create_group, join_group, recvgrp_msg,
			   error_msg, login_msg, exit_msg, who_msg,
			   invite_player, game_move, accept_invite, refuse_invite, invite_sent, invite_received, move_sent, move_received, won_game, lost_game, draw_game,
    		   file_msg, cont_msg, acptf_msg, rfsf_msg, recvg_msg, sentp_msg 
} messageType;

typedef struct {
    messageType type;
    char *id;
    person *sender;
    char *text;
    person *receiver;
    struct Group *group;
} message;

typedef struct {
    char name[30];
    int numberOfMembers;
    person *members[MAX_PERSON_PER_GROUP];
} Group;

int sendMessageToGroup (message *msg);
int sendMessageToPerson (message *msg);
person *getPersonByName (char *name);
person **getAllUsers (int *size);
int sendMessage (message *msg);
void setupDatabase ();

/******************************** Group ***************************/

Group *getGroupByName (char *name);
int addUserToGroup (Group *g, person *p);
void decreaseCounter(message *m);
