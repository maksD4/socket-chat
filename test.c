#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <hiredis/hiredis.h>
#include "data/mongo.h"

//gcc test.c data/mongo.c -I. -L. -lhiredis -Wl,-rpath='$ORIGIN' -o a.out $(pkg-config --cflags --libs libmongoc-1.0 libbson-1.0)
int main(){

    pid_t redis_pid = fork();
    if (redis_pid == 0) {
        execlp("redis-server", "redis-server", "--daemonize", "no", NULL);
        perror("execlp");
        exit(1);
    }

    sleep(5);

    redisContext *c = redisConnect("127.0.0.1", 6379);

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

    if(!mongodb_init()){
        printf("Mongodb has successfully set up!\n");
        mongodb_cleanup();
    }
    

    // Close the connection.
    redisFree(c);

    kill(redis_pid, SIGTERM);
    exit(EXIT_SUCCESS);
}