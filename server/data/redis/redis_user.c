#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/hiredis/hiredis.h>
#include <pthread.h>

#include "lib/constants.h"
#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_session.h"
#include "server/data/redis/redis_client.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/utils/models/user.h"

static pthread_mutex_t friend_mutex = PTHREAD_MUTEX_INITIALIZER;

int redis_check_friend_invite(int id1, int id2){
    redisContext *c = redis_get();

    if(c == NULL || c->err){
        return -1;
    }

    redisReply *r = redisCommand(c, "LPOS user:%d:friend_invite %d", id2, id1);

    if(r == NULL){
        printf("[%d][REDIS] >> Friend invite check failed!\n", getpid());
        return -1;
    }
    
    if(r->type == REDIS_REPLY_NIL){
        freeReplyObject(r);
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_add_friend_invite(int id1, int id2){
    redisContext *c = redis_get();

    if(c == NULL || c->err){
        return -1;
    }

    redisReply *r = redisCommand(c, "RPUSH user:%d:friend_invite %d", id2, id1);

    if(r == NULL){
        printf("[%d][REDIS] >> Adding friend to queue failed!\n", getpid());
        return -1;
    }
    freeReplyObject(r);
    
    // r = redisCommand(c, "RPUSH user:%d:friend_invite %d", id1, id2);

    // if(r == NULL){
    //     printf("[%d][REDIS] >> Adding friend to queue failed!\n", getpid());
    //     return -1;
    // }
    // freeReplyObject(r);

    printf("[REDIS] >> %d have invited %d\n", id1, id2);

    return 0;
}

int redis_remove_friend_invite(int id1, int id2){
    redisContext *c = redis_get();

    if(c == NULL || c->err){
        return -1;
    }

    redisReply *r = redisCommand(c, "LREM user:%d:friend_invite 0 %d", id2, id1);

    if(r == NULL){
        printf("[%d][REDIS] >> Removing friend from queue failed!\n", getpid());
        return -1;
    }
    freeReplyObject(r);
    
    // r = redisCommand(c, "LREM user:%d:friend_invite 0 %d", id1, id2);

    // if(r == NULL){
    //     printf("[%d][REDIS] >> Removing friend from queue failed!\n", getpid());
    //     return -1;
    // }

    // freeReplyObject(r);
    return 0;
}

// Save id, name, password, friends_num, chats_num HSET, but
// the friends and chats array save with RPUSH
// id of user in redis: "user:<id>", 
// identically id of user array in redis: "user:<id>:<array_name>"
int redis_user_write(user_t user){
    redisContext *c = redis_get();

    if (c == NULL || c->err) {
        return -1;
    }

    // Saving id, name, password, friends_num and chats_num in redis
    redisCommand(c, 
        "HSET user:%d  name %s password %s friends_num %d chats_num %d", 
        user.id, user.name, user.password, user.friends_num, user.chats_num);

    // Saving friends and chats array in redis
    for(int i = 0; i < user.friends_num; i++){
        redisCommand(c, "RPUSH user:%d:friends %s", user.id, mongodb_user_get_name(user.friends[i]));
        //printf("%s's %d friend name: %s\n", user.name, i, get_name(user.friends[i]));
    }

    for(int i = 0; i < user.chats_num; i++){
        redisCommand(c, "RPUSH user:%d:chats %d", user.id, user.chats[i]);
    }

    return 0;
}

// reads user from redis by session key
int redis_user_read(char *session, user_t *user){
    redisContext *c = redis_get();

    if (c == NULL || c->err) {
        return -1;
    }

    int id = redis_session_read(session);

    if(id == -1){
        printf("[%d][REDIS] >> SESSION READ FAILED WHILE USER READING!\n", getpid());
        return -1;
    }

    printf("session:%s id -> %d\n", session, id);
    user->id = id;

    redisReply *r = redisCommand(c, "HMGET user:%d name password friends_num chats_num", user->id);

    if(r->type != REDIS_REPLY_ARRAY || r->elements != 4){
        fprintf(stderr, "HMGET returned unexpected format!\n");
        freeReplyObject(r);
        return -1;
    }

    for(int i = 0; i < 4; i++){
        if(!r->element[i]->str){
            fprintf(stderr, "Invalid %d variable while reading from redis!\n", i);
            return -1;
        }
    }

    user->name = r->element[0]->str;
    user->password = r->element[1]->str;
    user->friends_num = atoi(r->element[2]->str);
    user->chats_num = atoi(r->element[3]->str);

    user->friends = malloc(user->friends_num * sizeof(int));
    user->chats = malloc(user->chats_num * sizeof(int));

    // extract char* friends names from user:<id>:friends
    r = redisCommand(c, "LRANGE user:%d:friends 0 -1", user->id);
    for(size_t i = 0; i < r->elements; i++){
        user->friends[i] = mongodb_user_get_id(r->element[i]->str);
    }

    
    r = redisCommand(c, "LRANGE user:%d:chats 0 -1", user->id);
    for(size_t i = 0; i < r->elements; i++){
        user->chats[i] = atoi(r->element[i]->str);
    }

    freeReplyObject(r);
    return 0;
}

int redis_user_exist(int id){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "EXISTS user:%d", id);

    if(r == NULL){
        printf("[%d][REDIS] >> USER EXIST CHECK FAILED!\n", getpid());
        return -1;
    }

    int exists = r->integer;
    freeReplyObject(r);

    return exists == 1 ? 0 : -1;
}

int redis_user_get_name(int id, char **name){
    if(redis_user_exist(id)){
        return -1;
    }

    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "HGET user:%d name", id);

    if(r == NULL || r->elements < 1){
        printf("[%d][REDIS] >> USER GET NAME ID: %d!\n", getpid(), id);
        return -1;
    }

    *name = strdup(r->element[0]->str);
    return 0;
}

int redis_add_friend(int id1, int id2){
    redisContext *c = redis_get();

    // redisCommand(c, "RPUSH user:%d:friends %s", user.id, mongodb_user_get_name(user.friends[i]));
    char *friend_name = mongodb_user_get_name(id2);
    redisReply *r = redisCommand(c, "RPUSH user:%d:friends %s", id1, friend_name);
    free(friend_name);
    freeReplyObject(r);

    r = redisCommand(c, "HINCRBY user:%d friends_num 1", id1);

    if(r == NULL){
        printf("[%d][REDIS] >> Incrementing friends_num failed!\n", getpid());
        return -1;
    }
    freeReplyObject(r);

    return 0;
}

int redis_user_socket_write(int id, char* session_key, int socket){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "HSET user:%d socket %d", id, socket);

    if(r == NULL){
        printf("[%d][REDIS] >> ID:SOCKET WRITE FAILED!\n", getpid());
        return -1;
    }
    freeReplyObject(r);

    // is user:<session> socket necessary?
    // r = redisCommand(c, "SET user:%s %d", session_key, socket);
    // if(r == NULL){
    //     printf("[%d][REDIS] >> KEY:SOCKET WRITE FAILED!\n", getpid());
    //     return -1;
    // }
    // freeReplyObject(r);

    r = redisCommand(c, "HSET socket:%d id %d session_key %s", socket, id, session_key);
    if(r == NULL){
        printf("[%d][REDIS] >> SOCKET:ID:KEY WRITE FAILED!\n", getpid());
        return -1;
    }
    freeReplyObject(r);

    return 0;
}

int redis_user_socket_read(int id){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "HGET user:%d socket", id);

    if(r == NULL){
        printf("[%d][REDIS] >> USER SOCKET READ FAILED!\n", getpid());
        return -1;
    }

    if(r->type == REDIS_REPLY_NIL){
        printf("[%d][REDIS] >> USER SOCKET NOT FOUND!\n", getpid());
        freeReplyObject(r);
        return -1;
    }

    int socket = atoi(r->str);
    freeReplyObject(r);
    return socket;
}

int redis_user_socket_get_id(int socket){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "HGET socket:%d id", socket);

    if(r == NULL){
        printf("[%d][REDIS] >> COULDN'T READ ID BY SOCKET!\n", getpid());
        return -1;
    }

    int id = atoi(r->str);
    freeReplyObject(r);
    return id;
}

char* redis_user_socket_get_session(int socket){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "HGET socket:%d session_key", socket);

    if(r == NULL){
        printf("[%d][REDIS] >> COULDN'T READ ID BY SOCKET!\n", getpid());
        return NULL;
    }

    if (r->type != REDIS_REPLY_STRING) {
        printf("[%d][REDIS] >> Unexpected reply type: %d\n", getpid(), r->type);
        freeReplyObject(r);
        return NULL;
    }

    // Validate length before copying
    if (r->len != SESSION_KEY_SIZE) {
        printf("[%d][REDIS] >> Invalid session length: %lld\n", getpid(), r->len);
        freeReplyObject(r);
        return NULL;
    }

    char* session_key = malloc(SESSION_KEY_SIZE + 1);
    memcpy(session_key, r->str, SESSION_KEY_SIZE);
    session_key[SESSION_KEY_SIZE] = '\0';

    freeReplyObject(r);
    return session_key;
}

int redis_user_cleanup(int id){
    redisContext *c = redis_get();

    if(c == NULL || c->err){
        printf("[%d][REDIS] >> Cleanup failed!\n", getpid());
        return -1;
    }

    redisReply *r = NULL;
    int errors = 0;

    // Delete socket from redis
    int socket = redis_user_socket_read(id);
    if(socket != -1){
        r = redisCommand(c, "DEL socket:%d", socket);
        if(r != NULL){
            freeReplyObject(r);
        }
    }
    else{
        printf("[%d][REDIS] >> Failed to read socket while cleaning up!\n", getpid());
        errors++;
    }

    // Delete user main hash (name, password, friends_num, chats_num, socket)
    r = redisCommand(c, "DEL user:%d", id);
    if(r == NULL){
        printf("[%d][REDIS] >> Failed to delete user:%d hash\n", getpid(), id);
        errors++;
    } 
    else{
        freeReplyObject(r);
    }

    // Delete friends
    r = redisCommand(c, "DEL user:%d:friends", id);
    if(r == NULL){
        printf("[%d][REDIS] >> Failed to delete user:%d:friends\n", getpid(), id);
        errors++;
    } 
    else{
        freeReplyObject(r);
    }

    // Delete chats
    r = redisCommand(c, "DEL user:%d:chats", id);
    if(r == NULL){
        printf("[%d][REDIS] >> Failed to delete user:%d:chats\n", getpid(), id);
        errors++;
    } 
    else{
        freeReplyObject(r);
    }

    if(errors == 0){
        printf("[%d][REDIS] >> User %d cleaned up successfully\n", getpid(), id);
        return 0;
    } else {
        printf("[%d][REDIS] >> User %d cleanup completed with %d errors\n", getpid(), id, errors);
        return -1;
    }
}

int redis_user_online(int id){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "SADD online %d", id);

    if(r == NULL){
        printf("[%d][REDIS] >> SETTING USER ONLINE FAILED!\n", getpid());
        return -1;
    }
    freeReplyObject(r);

    return 0;
}

int redis_user_offline(int id){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "SREM online %d", id);

    if(r == NULL){
        printf("[%d][REDIS] >> SETTING USER OFFLINE FAILED!\n", getpid());
        return -1;
    }
    freeReplyObject(r);
    
    if(redis_user_cleanup(id) == -1){
        printf("[%d][BRIDGE] >> REDIS USER CLEANUP FAILED!\n", getpid());
        return -1;
    }
    
    return 0;
}

int redis_is_user_online(int id){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "SISMEMBER online %d", id);

    if(r == NULL){
        printf("[%d][REDIS] >> USER ONLINE CHECK FAILED!\n", getpid());
        return -1;
    }

    int exist = r->integer;
    freeReplyObject(r);

    return exist == -1 ? -1 : exist - 1;
}

// Redis: adds chat_id to user's chats list
int redis_add_chat_to_user(int user_id, int chat_id){
    redisContext *c = redis_get();

    if(c == NULL || c->err){
        return -1;
    }

    redisReply *r = redisCommand(c, "RPUSH user:%d:chats %d", user_id, chat_id);

    if(r == NULL){
        printf("[%d][REDIS] >> Failed to add chat %d to user %d\n", getpid(), chat_id, user_id);
        return -1;
    }

    freeReplyObject(r);

    r = redisCommand(c, "HINCRBY user:%d chats_num 1", user_id);

    if(r == NULL){
        printf("[%d][REDIS] >> Failed to increment chats_num for user %d\n", getpid(), user_id);
        return -1;
    }

    freeReplyObject(r);
    return 0;
}