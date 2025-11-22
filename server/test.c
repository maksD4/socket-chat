#include <stdio.h>
#include <hiredis/hiredis.h>
#include "server/headers/data/mongo.h"
#include "server/headers/data/redis.h"
#include "server/headers/data/utils.h"

//gcc server/test.c server/data/*.c -I. -L. -lhiredis -Wl,-rpath='$ORIGIN' -o a.out $(pkg-config --cflags --libs libmongoc-1.0 libbson-1.0)
int main(){
    redis_init();

    if(!mongodb_init()){
        printf("Mongodb has successfully set up!\n");
        
    }

    int friends[1] = {2};
    int chats[1] = {1};
    bson_t user = bson_create_user(1, "Bob", "passwd", friends, 1, chats, 1);
    mongodb_insert("USER", user);
    bson_destroy(&user);

    int friends2[1] = {1};
    bson_t user2 = bson_create_user(2, "Alice", "p4ssw0rd", friends2, 1, chats, 1);
    mongodb_insert("USER", user2);
    bson_destroy(&user2);

    if(!load_user_to_redis("Bob")){
        printf("Bob was successfully transferred from db to redis!\n");
    }
    else{
        printf("Bob's transfer from db to redis has failed!\n");
    }
    
    redis_cleanup();
    sleep(3);
    mongodb_cleanup();
    exit(EXIT_SUCCESS);
}