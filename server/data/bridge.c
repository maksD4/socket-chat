#include <stdio.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "server/utils/constants.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/redis/redis_client.h"
#include "server/data/redis/redis_user.h"
#include "server/utils/models/models_print.h"
#include "server/utils/models/user.h"
#include "server/data/bridge.h"

int mongodb_to_redis(char* name){
    user_t user;
    if(mongodb_user_read(name, &user)){
        printf("[%d][DB] MONGODB USER READ FAIL!\n", getpid());
        return -1;
    }

    print_user(user);
    redis_user_write(user);
    
    


    //free(user.name);
    //free(user.password);
    free(user.friends);
    free(user.chats);
    return 0;
}