#include <stdio.h>
#include <stdlib.h>
#include "server/utils/models/user.h"

void free_user(user_t *user){
    if(user == NULL){
        return;
    }

    if(user->name != NULL){
        free(user->name);
        user->name = NULL;
    }

    if(user->password != NULL){
        free(user->password);
        user->password = NULL;
    }

    if(user->friends != NULL){
        free(user->friends);
        user->friends = NULL;
    }

    if(user->chats != NULL){
        free(user->chats);
        user->chats = NULL;
    }
}