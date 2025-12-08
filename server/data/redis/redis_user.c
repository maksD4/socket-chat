#include <stdio.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_session.h"
#include "server/data/redis/redis_client.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/utils/models/user.h"

int get_user_id(const char* name){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("name", BCON_UTF8(name));
    bson_t *opts = BCON_NEW("projection", "{",
                    "_id", BCON_BOOL(false),
                    "id", BCON_BOOL(true),
                    "}");
    mongodb_get_doc("USER", filter, opts, &doc);

    if(!doc){
        return -1;
    }

    if(bson_empty(doc)){
        return -1;
    }        

    if(!bson_has_field(doc, "id")){
        return -1;
    }

    int id;
    bson_iter_t iter;
    if(bson_iter_init_find(&iter, doc, "id") && BSON_ITER_HOLDS_INT32(&iter)){
        id = bson_iter_int32(&iter);
    }
    else{
        return -1;
    }

    //printf("get_user_id(%s): %d\n", name, id);

    bson_destroy(filter);
    bson_destroy(opts);
    return id;
}

char* get_name(int id){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("id", BCON_INT32(id));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "name", BCON_BOOL (true),
                    "}");
    if(mongodb_get_doc("USER", filter, opts, &doc)){
        return "Unknown";
    }
    
    char *name;
    bson_iter_t iter;
    if(bson_iter_init_find(&iter, doc, "name") && BSON_ITER_HOLDS_UTF8(&iter)){
        uint32_t len;
        name = bson_iter_utf8(&iter, &len);
    }
    else{
        return "Unknown";
    }

    //printf("get_name(%d): %s\n", id, name);

    bson_destroy(filter);
    bson_destroy(opts);
    return name;
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
        redisCommand(c, "RPUSH user:%d:friends %s", user.id, get_name(user.friends[i]));
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
        user->friends[i] = get_user_id(r->element[i]->str);
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
        printf("[%d][REDIS] >> USERNAME READ FAILED!\n", getpid());
        return -1;
    }

    *name = r->element[0]->str;
    return 0;
}