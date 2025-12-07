#include <stdio.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "lib/constants.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/mongodb/mongodb_room.h"
#include "server/data/redis/redis_client.h"
#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_room.h"
#include "server/utils/models/models_print.h"
#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/data/bridge.h"

int mongodb_to_redis(char* name){
    user_t user;
    if(mongodb_user_read(name, &user)){
        printf("[%d][DB] MONGODB USER READ FAIL!\n", getpid());
        return -1;
    }

    for(int i = 0; i < user.chats_num; i++){
        if(!redis_room_exist(user.chats[i])){
            printf("[DB] >> Chat %d is already imported to redis!\n", user.chats[i]);
            continue;
        }

        room_t chat;
        if(mongodb_room_read(user.chats[i], &chat)){
            printf("[%d][DB] >> MONGODB ROOM READ FAIL!\n", getpid());
            break;
        }
        print_room(chat);
        redis_room_write(chat);
    }

    print_user(user);
    redis_user_write(user);
    
    for(int i = 0; i < user.chats_num; i++){
        if(!redis_room_exist(user.chats[i])){
            room_t test_room;
            redis_room_read(user.chats[i], &test_room);
            print_room(test_room);
        }
    }

    //free(user.name);
    //free(user.password);
    free(user.friends);
    free(user.chats);
    return 0;
}


// send user data from redis to mongodb
// for chat : user.chats
// check if any user from chat is in redis
//  if yes dont transfer chat from redis to mongodb
//  if no transfer chat from redis to mongodb
int redis_to_mongodb(char *session){
    user_t user;
    if(redis_user_read(session, &user)){
        printf("[%d][REDIS] >> USER READ ERROR WHILE TRANSFERRING FROM REDIS TO DB!\n", getpid());
        return -1;
    }

    if(mongodb_user_write(user)){
        printf("[%d][DB] >> USER WRITE ERROR WHILE TRANSFERRING FROM REDIS TO DB\n", getpid());
        return -1;
    }

    for(int i = 0; i < user.chats_num; i++){
        if(redis_room_exist(user.chats[i])){
            printf("[%d][REDIS] >> ROOM WITH %d ID DOESNT EXIST!\n", getpid(), user.chats[i]);
            return -1;
        }
        room_t room;
        if(redis_room_read(user.chats[i], &room)){
            printf("[%d][REDIS] >> ROOM READ FAILED!\n", getpid());
            return -1;
        }
        
        // check if any user of room is online
        if(!mongodb_room_any_online(user.id, &room)){
            continue;
        }
        
        if(mongodb_room_write(room)){
            printf("[%d][DB] >> ROOM WRITE FAILED!\n", getpid());
            return -1;
        }
    }
}
/*
int sync_mongodb(){

}
*/

