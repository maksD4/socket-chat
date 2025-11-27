#include <stdio.h>
#include "server/utils/models/user.h"

void print_user(user_t user){
    printf("user_id: %d\n", user.user_id);
    printf("name: %s\n", user.name);
    printf("Password: %s\n", user.password);

    printf("Friends (%d): ", user.friends_num);
    for(int i = 0; i < user.friends_num; i++){
        printf("%d ", user.friends[i]);
    }

    printf("\nChats (%d): ", user.chats_num);
    for(int i = 0; i < user.chats_num; i++){
        printf("%d ", user.chats[i]);
    }

    printf("\n");
}