#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> 
#include <hiredis/hiredis.h>
#include "server/headers/constants.h"
#include "server/headers/data/redis.h"
#include "server/headers/data/utils.h"
#include "server/headers/data/user.h"

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

// Save user_id, name, password, friends_num, chats_num HSET, but
// the friends and chats array save with RPUSH
// id of user in redis: "user:<id>", 
// identically id of user array in redis: "user:<id>:<array_name>"
void redis_write_user(user_t user){
    // Saving user_id, name, password, friends_num and chats_num in redis
    redisCommand(c, 
        "HSET user:%d  name %s password %s friends_num %d chats_num %d", 
        user.user_id, user.name, user.password, user.friends_num, user.chats_num);

    // Saving friends and chats array in redis
    for(int i = 0; i < user.friends_num; i++){
        redisCommand(c, "RPUSH user:%d:friends %s", user.user_id, get_name(user.friends[i]));
        printf("%s's %d friend name: %s\n", user.name, i, get_name(user.friends[i]));
    }

    for(int i = 0; i < user.chats_num; i++){
        redisCommand(c, "RPUSH user:%d:chats %d", user.user_id, user.chats[i]);
    }
}

// reads user from redis by session key
int redis_read_user(char *session, user_t *user){
    printf("asd2\n");
    redisReply *r = redisCommand(c, "HGET session:%s user_id", session);

    printf("session(%s): %s\n", session, r->str);
    user->user_id = r->str ? atoi(r->str) : 0;

    if(!user->user_id){
        fprintf(stderr, "Invalid user_id while reading from redis!\n");
        return -1;
    }

    r = redisCommand(c, "HMGET user:%d name password friends_num chats_num", user->user_id);

    if(r->type != REDIS_REPLY_ARRAY || r-> elements != 4){
        fprintf(stderr, "HMGET returned unexpected format!\n");
        freeReplyObject(r);
        return -1;
    }

    for(int i = 0; i < 4; i++){
        if(!r->element[i]->str){
            fprintf(stderr, "Invalid %d variable while reading from redis!\n");
            return -1;
        }
    }

    user->name = r->element[0]->str;
    user->password = r->element[1]->str;
    user->friends_num = atoi(r->element[2]->str);
    user->chats_num = atoi(r->element[3]->str);

    user->friends = malloc(user->friends_num * sizeof(int));
    user->chats = malloc(user->chats_num * sizeof(int));
    printf("asd3\n");
    // extract char* friends names from user:<id>:friends
    r = redisCommand(c, "LRANGE user:%d:friends 0 -1", user->user_id);
    for(size_t i = 0; i < r->elements; i++){
        user->friends[i] = get_user_id(r->element[i]->str);
    }

    
    r = redisCommand(c, "LRANGE user:%d:chats 0 -1", user->user_id);
    for(size_t i = 0; i < r->elements; i++){
        user->chats[i] = atoi(r->element[i]->str);
    }
    printf("asd4\n");
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
    // Close the connection.
    printf("[%d] asd\n", getpid());

    redisReply *r = redisCommand(c, "FLUSHALL");
    if(!strcmp(r->str, "OK")){
        printf("Redis has been successfully wiped out!\n");
    }   
    printf("redis_pid: %d\nthis: %d\n", redis_pid, getpid());

    freeReplyObject(r);
    redisFree(c);
    kill(redis_pid, SIGTERM);
    printf("asd2\n");
}