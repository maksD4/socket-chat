#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "server/data/mongodb/mongodb_user.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/utils/models/user.h"
#include "server/utils/models/models_print.h"
#include "lib/constants.h"

bson_t bson_create_user(user_t user){
    bson_t doc;
    bson_init(&doc);

    BSON_APPEND_INT32(&doc, "id", user.id);
    BSON_APPEND_UTF8(&doc, "name", user.name);
    BSON_APPEND_UTF8(&doc, "password", user.password);

    bson_t friends;
    bson_init(&friends);
    char key[12];
    for(int i = 0; i < user.friends_num; i++){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&friends, key, user.friends[i]);
    }
    BSON_APPEND_ARRAY(&doc, "friends", &friends);
    BSON_APPEND_INT32(&doc, "friends_num", user.friends_num);

    bson_t chats;
    bson_init(&chats);
    for(int i = 0; i < user.chats_num; i++){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&chats, key, user.chats[i]);
    }
    BSON_APPEND_ARRAY(&doc, "chats", &chats);
    BSON_APPEND_INT32(&doc, "chats_num", user.chats_num);

    BSON_APPEND_NOW_UTC(&doc, "last");

    return doc;
}

int mongodb_user_get_id(const char* name){
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

char* mongodb_user_get_name(int id){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("id", BCON_INT32(id));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "name", BCON_BOOL (true),
                    "}");
    if(mongodb_get_doc("USER", filter, opts, &doc)){
        bson_destroy(filter);
        bson_destroy(opts);
        return strdup("Unknown");
    }
    
    char *result;
    bson_iter_t iter;
    if(bson_iter_init_find(&iter, doc, "name") && BSON_ITER_HOLDS_UTF8(&iter)){
        uint32_t len;
        const char* name = bson_iter_utf8(&iter, &len);
        result = strdup(name);
    }
    else{
        result = strdup("Unknown");
    }

    //printf("get_name(%d): %s\n", id, name);

    bson_destroy(filter);
    bson_destroy(opts);
    return result;
}

int mongodb_user_read(char *name, user_t *user){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("name", BCON_UTF8(name));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "id", BCON_BOOL (true),
                    "name", BCON_BOOL (true),
                    "password", BCON_BOOL (true),
                    "friends", BCON_BOOL (true),
                    "friends_num", BCON_BOOL (true),
                    "chats", BCON_BOOL (true),
                    "chats_num", BCON_BOOL (true),
                    "last", BCON_BOOL (true),
                    "}");

    if(mongodb_get_doc("USER", filter, opts, &doc)){
        printf("[%d][DB] >> COULDNT FIND USER WITH %s USERNAME!\n", getpid(), name);
        return -1;
    }

    bson_destroy(filter);
    bson_destroy(opts);

    bson_iter_t iter;

    user->name = strdup(name);

    if(bson_iter_init_find(&iter, doc, "id") && BSON_ITER_HOLDS_INT32(&iter)) {
        user->id = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER ID FIND FAILED!\n", getpid(), name);
        return -1;
    }

    if(bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter)) {
        //uint32_t len;
        //user->password = bson_iter_utf8(&iter, &len);
        user->password = strdup(bson_iter_utf8(&iter, NULL));
        //printf("mongodb_user_read %s passwd: %s\n", user->name, user->password);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER PASSWORD FIND FAILED!\n", getpid(), name);
        return -1;
    } 

    if(bson_iter_init_find(&iter, doc, "friends_num") && BSON_ITER_HOLDS_INT32(&iter)) {
        user->friends_num = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER FRIENDS NUMBER FIND FAILED!\n", getpid(), name);
        return -1;
    }

    user->friends = NULL;
    user->friends = malloc(user->friends_num * sizeof(int));
    if(bson_iter_init_find(&iter, doc, "friends") && BSON_ITER_HOLDS_ARRAY(&iter)) {

        uint32_t arr_len;
        const uint8_t *arr_data;
        bson_iter_array(&iter, &arr_len, &arr_data);

        bson_t arr;
        bson_init_static(&arr, arr_data, arr_len);

        bson_iter_t arr_iter;
        bson_iter_init(&arr_iter, &arr);

        int i = 0;
        while (bson_iter_next(&arr_iter) && i < FRIENDS_MAX) {
            if (BSON_ITER_HOLDS_INT32(&arr_iter)) {
                user->friends[i] = bson_iter_int32(&arr_iter);
                i++;
            }
        }
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER FRIENDS FIND FAILED!\n", getpid(), name);
        return -1;
    }

    if(bson_iter_init_find(&iter, doc, "chats_num") && BSON_ITER_HOLDS_INT32(&iter)){
        user->chats_num = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER CHATS NUMBER FIND FAILED!\n", getpid(), name); 
    }

    user->chats = NULL;
    user->chats = malloc(user->chats_num * sizeof(int));
    if (bson_iter_init_find(&iter, doc, "chats") && BSON_ITER_HOLDS_ARRAY(&iter)) {

        uint32_t arr_len;
        const uint8_t *arr_data;
        bson_iter_array(&iter, &arr_len, &arr_data);

        bson_t arr;
        bson_init_static(&arr, arr_data, arr_len);

        bson_iter_t arr_iter;
        bson_iter_init(&arr_iter, &arr);

        int i = 0;
        while (bson_iter_next(&arr_iter) && i < ROOM_MAX) {
            if (BSON_ITER_HOLDS_INT32(&arr_iter)) {
                user->chats[i++] = bson_iter_int32(&arr_iter);
            }
        }
    }
    else{
        fprintf(stderr, "[%d][DB] >> %s's USER CHATS FIND FAILED!\n", getpid(), name);
        return -1;
    }

    bson_destroy(doc);
    return 0;
}

int mongodb_user_write(user_t user){
    bson_t bson = bson_create_user(user);
    // change every id name of object to "id", then in mongodb_insert check if id of object already exists
    // if yes then update, if not then insert
    if(mongodb_insert("USER", bson) == -1){
        printf("[%d][DB] >> MONGODB %s INSERT FAILED!\n", getpid(), user.name);
        return -1;
    }

    mongodb_print_collection("USER");

    bson_destroy(&bson);
    return 0;
}