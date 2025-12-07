#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <hiredis/hiredis.h>

#include "server/data/redis/redis_client.h"
#include "lib/constants.h"

static const char *hex_digits = "0123456789abcdef";

int redis_session_exist(const char *session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "HEXISTS session:%s id", session);

    if(r == NULL){
        printf("[%d][REDIS] >> REDIS EXIST REPLY IS NULL: %s\n", getpid(), c->errstr);
        return -1;
    }

    if(r->type == REDIS_REPLY_ERROR){
        printf("[%d][REDIS] >> REDIS EXIST REPLY ERROR: %s\n", getpid(), r->str);
        freeReplyObject(r);
        return -1;
    }

    int exist = (int) r->integer;
    printf("[%d][REDIS] >> HEXISTS session:%s doesExist -> %lld\n", getpid(), session, r->integer);
 	freeReplyObject(r);
    return exist == 1 ? 0 : -1;
}

int redis_session_write(char **session_key, int id){
    *session_key = malloc((SESSION_KEY_SIZE + 1) * sizeof(char));
    srand(time(NULL));

    for(int i = 0; i < SESSION_KEY_SIZE; i++){
        (*session_key)[i] = hex_digits[rand()%16];
    }
    (*session_key)[SESSION_KEY_SIZE] = '\0';

    redisContext *c = redis_get();

    if(c == NULL){
        printf("c is NULL\n");
    }

    redisReply *r = redisCommand(c, "HSET session:%s id %d", *session_key, id);

    if(!r){
        printf("[%d][REDIS] >> REDIS REPLY IS NULL WHILE WRITING!\n", getpid());
        return -1;
    }

    if(r->integer == 0){
        printf("[%d][REDIS] >> SESSION INSERT FAIL!\n", getpid());
        return -1;
    }    

    printf("[%d][REDIS] >> session:%s insert.\n", getpid(), *session_key);
    freeReplyObject(r);
    return 0;
}

// Get id with session token
int redis_session_read(const char *session){
    if(redis_session_exist(session)){
        return -1;
    }

    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "HGET session:%s id", session);

    int id = r->str ? atoi(r->str) : -1;

    freeReplyObject(r);
    return id;
}


int redis_session_delete(const char *session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "HDEL session:%s id", session);

    if(r->integer == 1 && r != NULL){
        printf("[%d][REDIS] >> Session:%s has been successfully deleted.\n", getpid(), session);
    }
    else{
        printf("[%d][REDIS] >> SESSION DELETION FAIL!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return 0;
}