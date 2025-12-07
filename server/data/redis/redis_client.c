#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <hiredis/hiredis.h>
#include "lib/constants.h"
#include "server/data/redis/redis_client.h"
#include "server/utils/models/user.h"

pid_t redis_pid;
redisContext *c;

/*
USER
int user_id
char* name
char* password
int* firends_id
int friend_num
int* chats_id
int chats_num
*/

redisContext* redis_get(){
    return c;
}

int redis_init(){
    redis_pid = fork();
    if (redis_pid == 0) {
        execlp("redis-server", "redis-server", "--daemonize", "no", NULL);
        perror("execlp");
        exit(1);
    }

    sleep(3);

    c = redisConnect("127.0.0.1", 6379);

    // Check if the context is null or if a specific
    // error occurred.
    if (c == NULL || c->err) {
        if (c != NULL) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }

        exit(1);
    }

    redisReply *r = redisCommand(c, "FLUSHALL");
    if(!strcmp(r->str, "OK")){
        printf("Redis has been successfully wiped out!");
    }

    // Set a string key.
    redisReply *reply = redisCommand(c, "SET foo bar");
    printf("Reply: %s\n", reply->str); // >>> Reply: OK
    freeReplyObject(reply);

    // Get the key we have just stored.
    reply = redisCommand(c, "GET foo");
    printf("Reply: %s\n", reply->str); // >>> Reply: bar
    freeReplyObject(reply);
    return 0;
}

/*
SETTING SESSION IN REDIS AND GETTING OUT OF REDIS
    redisCommand(c, "HSET session:%s user_id %d", "bobsession", get_user_id("Bob"));
    user_t user;
    if(!redis_read_user("bobsession", &user)){
        printf("%s successfully read data with session key!\n", "bobsession");
        print_user(user);
    }
    else{
        fprintf(stderr, "redis user read error\n");
    }
    */


void redis_cleanup(){
    // Sync data with mongodb

    // Clean redis
    redisReply *r = redisCommand(c, "FLUSHALL");
    if(!strcmp(r->str, "OK")){
        printf("Redis has been successfully wiped out!\n");
    }   

    freeReplyObject(r);
    redisFree(c);
    kill(redis_pid, SIGTERM);
    printf("[%d] Redis has been successfully cleaned up!\n", getpid());
}