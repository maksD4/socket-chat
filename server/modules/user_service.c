#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/mongodb/mongodb_room.h"
#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_session.h"
#include "server/data/redis/redis_room.h"
#include "server/utils/models/models_print.h"
#include "lib/constants.h"


int account_creation(char* name, char* password){
    if(mongodb_user_get_id(name) > 0){
        printf("[%d][SERVICE] >> USER WITH %s NAME ALREADY EXIST!\n", getpid(), name);
        return -1;
    }

    int id = get_next_id("USER");
    if(id == -1){
        printf("[%d][SERVICE] >> NEXT ID IS EQUAL -1!\n", getpid());
        return -1;
    }

    user_t user = create_user(id, name, password, NULL, 0, NULL, 0);
    
    if(mongodb_user_write(user)){
        printf("[%d][SERVICE] >> DB USER WRITE FAILED!\n", getpid());
        return -1;
    }

    return 0;
}

// return freshly created id of chat, -1 if it fails
int room_creation(int usr_amount, int* users){
    int chat_id = get_next_id("ROOM");
    if(chat_id == -1){
        printf("[%d][SERVICE] >> ROOM NEXT ID IS EQUAL -1\n", getpid());
        return -1;
    }

    room_t room = create_room(chat_id, users, usr_amount, NULL, 0);

    if(mongodb_room_write(room) == -1){
        printf("[%d][SERVICE] >> DB ROOM WRITE FAILED!\n", getpid());
        return -1;
    }

    if(redis_room_write(room) == -1){
        printf("[%d][SERVICE] >> REDIS ROOM WRITE FAILED!\n", getpid());
        return -1;
    }

    return chat_id;
}

int auth(char* name, char* password, int* user_id){
    // Check if user exist
    if(mongodb_user_get_id(name) < 1){
        return -1;
    }
    user_t user;

    if(mongodb_user_read(name, &user)){
        printf("[%d][AUTH] >> DB USER READ FAIL!\n", getpid());
        *user_id = -1;
        return -1;
    }

    if(!strcmp(password, user.password)){
        memcpy(user_id, &user.id, sizeof(user.id));
        return 0;
    }

    *user_id = -1;
    return -1;
}

/*
int friend_add(char* session_key, char* name){

}
int friend_remove(char* session_key, char* name);
*/