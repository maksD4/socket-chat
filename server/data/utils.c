#include <stdio.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "server/headers/constants.h"
#include "server/headers/data/mongo.h"
#include "utils.h"

void load_user_to_redis(const char* name){
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
    int user_id;
    char *password;
    int friends_num;
    int chats_num;
    bson_iter_t iter;

    if(bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_INT32(&iter)) {
        user_id = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "user_id eror");
    }

    if(bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter)) {
        uint32_t len;
        password = bson_iter_utf8(&iter, &len);
    }
    else{
        fprintf(stderr, "password eror");
    } 

    if(bson_iter_init_find(&iter, doc, "friends_num") && BSON_ITER_HOLDS_INT32(&iter)) {
        friends_num = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "friends num eror");
    }

    int friends[friends_num];
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
                friends[i] = bson_iter_int32(&arr_iter);
                i++;
            }
        }
    }
    else{
        fprintf(stderr, "friends eror");
    }

    if(bson_iter_init_find(&iter, doc, "chats_num") && BSON_ITER_HOLDS_INT32(&iter)){
        chats_num = bson_iter_int32(&iter);
    }

    int chats[chats_num];
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
                chats[i++] = bson_iter_int32(&arr_iter);
            }
        }
    }
    else{
        fprintf(stderr, "chats eror");
    }


    printf("user_id: %d\n", user_id);
    printf("name: %s\n", name);
    printf("Password: %s\n", password);

    printf("Friends (%d): ", friends_num);
    for(int i = 0; i < friends_num; i++){
        printf("%d ", friends[i]);
    }

    printf("\nChats (%d): ", chats_num);
    for(int i = 0; i < chats_num; i++){
        printf("%d ", chats[i]);
    }

    printf("\n");

    char *str = bson_as_canonical_extended_json(doc, NULL);
    printf("load_to_redis:\n%s\n", str);
    bson_free(str);
}