#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> 
#include <hiredis/hiredis.h>
#include "server/headers/constants.h"
#include "server/headers/data/redis.h"

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
    pid_t redis_pid = fork();
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
int save_user(int user_id, char* name, char* password, int* firends, int friends_num, int* chats, int chats_num){
    char key[5+ID_SIZE];
    snprintf(key, sizeof(key), "user:%d", user_id);

    char id_str[ID_SIZE], friends_num_str[FRIENDS_NUM_SIZE], chats_num_str[ROOM_NUM_SIZE];
    snprintf(id_str, sizeof(id_str), "%d", user_id);
    snprintf(friends_num_str, sizeof(friends_num_str), "%d", friends_num);
    snprintf(chats_num_str, sizeof(chats_num_str), "%d", chats_num);

    char friends_str[(ID_SIZE + 1) * FRIENDS_MAX];
    for(int i = 0; i < friends_num; i++){
        char buf[ID_SIZE];
        snprintf(buf, sizeof(buf), )
    }



}
*/

void redis_cleanup(){
    // Close the connection.
    redisFree(c);

    kill(redis_pid, SIGTERM);
}