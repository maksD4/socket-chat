#include <stdio.h>
#include <stdlib.h>
#include "server/utils/models/user.h"

void free_user(user_t *user){
    if(user == NULL){
        return;
    }
    printf("user != null\n");

    if(user->name != NULL){
        printf("username != null\n");
        free(user->name);
        user->name = NULL;
    }

    if(user->password != NULL){
        printf("password != null\n");
        free(user->password);
        user->password = NULL;
    }

    if(user->friends != NULL){
        printf("friends != null\n");
        free(user->friends);
        user->friends = NULL;
    }

    if(user->chats != NULL){
        printf("chats != null\n");
        free(user->chats);
        user->chats = NULL;
    }
}