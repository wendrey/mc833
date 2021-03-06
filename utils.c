
#include "utils.h"

unsigned int hash (char *string) {
    int hash = 5381;
    int c;
    
    while ((c = *string++))
        hash = ((hash << 5) + hash) + c;
    
    return (unsigned int) hash % USER_HASH_TABLE_SIZE;
}

char* getNWord (char *string, int n) {
    int countWords = 0;
    int boolean = 0;
    int i;   
    int start = 0, end = 0;

    if (!n) {
		for (i = 0; i < strlen(string); i++) { 
			if (string[i] == '"' && !boolean) {
				start = i+1;
				boolean = 1;
			}
			else if (string[i] == '"' && boolean)
				end = i-1;
		}
    } 
   
    for (i = 0; n != 0; i++) {
        if (string[i] != ' ' && string[i] != '\0' && string[i] != '\n') {
            if (boolean == 0) {
                countWords++;
                if (countWords == n) {
                    start = i;
                }
            }
            
            boolean = 1;
        }
        else if (string[i] == ' ' || string[i] == '\0' || string[i] == '\n') {
            boolean = 0;
            if (countWords == n) {
                end = i-1;
                break;
            }
            
        }
    }

    char *word = malloc((end - start + 2)*sizeof(char));
    
    if (!n && start == end) {
		word[0] = '\0';
		return word;
    }

    for (i = 0; i <= end-start + 1; i++) {
        word[i] = string[start + i];
    }
    word[end-start+1] = '\0';
    
    return word;
}

void upperCaseString (char*string) {
    
    int i = 0;
    while (string[i] != '\0') {
        if (string[i] >= 'a' && string[i] <= 'z')
            string[i] += 'A' - 'a';
        i++;
    }
    
}

int GetMessageType (char *string) {

	if (!strcmp(string,"LOGIN"))
		return login_msg;
	if (!strcmp(string,"SEND"))
		return new_msg;
	if (!strcmp(string,"SENDG"))
		return newgrp_msg;
	if (!strcmp(string,"CREATEG"))
		return create_group;
	if (!strcmp(string,"JOING"))
		return join_group;
	if (!strcmp(string,"WHO"))
		return who_msg;
	if (!strcmp(string,"EXIT"))
		return exit_msg;
	if (!strcmp(string,"GAME"))
		return invite_player;
	if (!strcmp(string,"GAMEA"))
		return game_move;
	if (!strcmp(string,"GAMEYES"))
		return accept_invite;
	if (!strcmp(string,"GAMENO"))
		return refuse_invite;
	
	return -1;
		
}

