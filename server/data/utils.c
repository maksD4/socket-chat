#include <stdio.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "server/headers/constants.h"
#include "server/headers/data/mongo.h"
#include "server/headers/data/redis.h"
#include "utils.h"

int get_user_id(const char* name){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("name", BCON_UTF8(name));
    bson_t *opts = BCON_NEW("projection", "{",
                    "_id", BCON_BOOL(false),
                    "user_id", BCON_BOOL(true),
                    "}");
    mongodb_get_doc("USER", filter, opts, &doc);

    int user_id;
    bson_iter_t iter;
    if(bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_INT32(&iter)){
        user_id = bson_iter_int32(&iter);
    }
    else{
        return -1;
    }

    printf("get_user_id(%s): %d\n", name, user_id);

    bson_destroy(filter);
    bson_destroy(opts);
    return user_id;
}

char* get_name(int user_id){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("user_id", BCON_INT32(user_id));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "name", BCON_BOOL (true),
                    "}");
    mongodb_get_doc("USER", filter, opts, &doc);
    
    char *name;
    bson_iter_t iter;
    if(bson_iter_init_find(&iter, doc, "name") && BSON_ITER_HOLDS_UTF8(&iter)){
        uint32_t len;
        name = bson_iter_utf8(&iter, &len);
    }
    else{
        return "-1";
    }

    printf("get_name(%d): %s\n", user_id, name);

    bson_destroy(filter);
    bson_destroy(opts);
    return name;
}

int load_user_to_redis(const char* name){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("name", BCON_UTF8(name));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "user_id", BCON_BOOL (true),
                    "name", BCON_BOOL (true),
                    "password", BCON_BOOL (true),
                    "friends", BCON_BOOL (true),
                    "friends_num", BCON_BOOL (true),
                    "chats", BCON_BOOL (true),
                    "chats_num", BCON_BOOL (true),
                    "last", BCON_BOOL (true),
                    "}");

    mongodb_get_doc("USER", filter, opts, &doc);

    bson_destroy(filter);
    bson_destroy(opts);

    user_t user;
    bson_iter_t iter;

    user.name = name;

    if(bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_INT32(&iter)) {
        user.user_id = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "user_id eror");
        return -1;
    }

    if(bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter)) {
        uint32_t len;
        user.password = bson_iter_utf8(&iter, &len);
    }
    else{
        fprintf(stderr, "password eror");
        return -1;
    } 

    if(bson_iter_init_find(&iter, doc, "friends_num") && BSON_ITER_HOLDS_INT32(&iter)) {
        user.friends_num = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "friends num eror");
        return -1;
    }

    user.friends = malloc(user.friends_num * sizeof(int));
    if (bson_iter_init_find(&iter, doc, "friends") && BSON_ITER_HOLDS_ARRAY(&iter)) {

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
                user.friends[i] = bson_iter_int32(&arr_iter);
                i++;
            }
        }
    }
    else{
        fprintf(stderr, "friends eror");
        return -1;
    }

    if(bson_iter_init_find(&iter, doc, "chats_num") && BSON_ITER_HOLDS_INT32(&iter)){
        user.chats_num = bson_iter_int32(&iter);
    }

    user.chats = malloc(user.chats_num * sizeof(int));
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
                user.chats[i++] = bson_iter_int32(&arr_iter);
            }
        }
    }
    else{
        fprintf(stderr, "chats eror");
        return -1;
    }


    printf("user_id: %d\n", user.user_id);
    printf("name: %s\n", user.name);
    printf("Password: %s\n", user.password);

    printf("Friends (%d): ", user.friends_num);
    for(int i = 0; i < user.friends_num; i++){
        printf("%d ", user.friends[i]);
    }

    printf("\nChats (%d): ", user.chats_num);
    for(int i = 0; i < user.chats_num; i++){
        printf("%d ", user.chats[i]);
    }

    printf("\n");

    char *str = bson_as_canonical_extended_json(doc, NULL);
    printf("load_to_redis:\n%s\n", str);
    bson_free(str);

    redis_write_user(user);
    
    /*
    free(user.name);
    free(user.password);
    free(user.friends);
    free(user.chats);
    */
    return 0;
}