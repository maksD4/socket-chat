#include <stdio.h>
#include <stdlib.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "mongo.h"

mongoc_client_t *client;
mongoc_database_t *database;

int mongodb_init(){
    mongoc_init();

    client = mongoc_client_new ("mongodb://root:root@localhost:27017/");
    if(!client){
        fprintf(stderr, "Faile to create client\n");
        return -1;
    }

    database = mongoc_client_get_database(client, "chat_database");

    bson_t reply;
    bson_error_t error;

    bool client_status = mongoc_client_get_server_status(client, NULL, &reply, &error);

    if(!client_status){
        fprintf(stderr, "Status error: %s\n", error.message);
        mongodb_cleanup();
        return -1;
    }

    // if collection exist then coll is NULL
    mongoc_collection_t *coll = mongoc_database_create_collection(database, "USER", NULL, &error);

    if(coll){
        printf("Collection USER has been created!\n");
    }

    printf("status: %d\n", client_status);

    // test insert
    int friends[3] = {2, 3, 0};
    int chats[2] = {1, 0};
    bson_t user = bson_create_user(1, "Bob", "passwd", friends, chats);
    mongodb_insert("USER", user);

    // print USER collection
    mongodb_print_collection(coll);

    bson_destroy(&user);
    mongoc_collection_destroy(coll);
    bson_destroy(&reply);
    return 0;
}

void mongodb_cleanup(){
    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}

void mongodb_insert(const char *collection_name, bson_t document){
    bson_error_t error;
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "chat_database", collection_name);

    if(!mongoc_collection_insert_one(collection, &document, NULL, NULL, &error)){
        fprintf(stderr, "Operation insert failed: %s\n", error.message);
    }

    mongoc_collection_destroy(collection);
}

void mongodb_print_collection(mongoc_collection_t *collection){
    bson_t *query = bson_new();

    mongoc_cursor_t *results = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc;

    while(mongoc_cursor_next(results, &doc)){
        char *str = bson_as_canonical_extended_json(doc, NULL);
        printf("%s\n", str);
        bson_free(str);
    }

    bson_error_t error;
    if (mongoc_cursor_error(results, &error)) {
        fprintf(stderr, "Cursor Error: %s\n", error.message);
    }

    mongoc_cursor_destroy(results);
    bson_destroy(query);
}

bson_t bson_create_user(int user_id, const char* name, const char* password, int* friends_id, int* chats_id){
    bson_t doc;
    bson_init(&doc);

    BSON_APPEND_INT32(&doc, "user_id", user_id);
    BSON_APPEND_UTF8(&doc, "name", name);
    BSON_APPEND_UTF8(&doc, "password", password);

    bson_t friends;
    bson_init(&friends);
    int i = 0;
    char key[12];
    while(friends_id[i] != 0){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&friends, key, friends_id[i]);
        i++;
    }
    BSON_APPEND_ARRAY(&doc, "friends", &friends);

    bson_t chats;
    bson_init(&chats);
    i = 0;
    while(chats_id[i] != 0){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&chats, key, chats_id[i]);
        i++;
    }
    BSON_APPEND_ARRAY(&doc, "chats", &chats);

    //int64_t now_ms = (int64_t) time(NULL) * 1000;
    //BSON_APPEND_DATE(&doc, "last", now_ms);
    BSON_APPEND_NOW_UTC(&doc, "last");
    
    return doc;
}

bson_t bson_create_user_num(int user_id, const char* name, const char* password, int* friends_id, int num_friends, int* chats_id, int num_chats){
    bson_t doc;
    bson_init(&doc);

    BSON_APPEND_INT32(&doc, "user_id", user_id);
    BSON_APPEND_UTF8(&doc, "name", name);
    BSON_APPEND_UTF8(&doc, "password", password);

    bson_t friends;
    bson_init(&friends);
    char key[12];
    for(int i = 0; i < num_friends; i++){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&friends, key, friends_id[i]);
    }
    BSON_APPEND_ARRAY(&doc, "friends", &friends);

    bson_t chats;
    bson_init(&chats);
    for(int i = 0; i < num_chats; i++){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&chats, key, chats_id[i]);
    }
    BSON_APPEND_ARRAY(&doc, "chats", &chats);

    //int64_t now_ms = (int64_t) time(NULL) * 1000;
    //BSON_APPEND_DATE(&doc, "last", now_ms);
    BSON_APPEND_NOW_UTC(&doc, "last");
    
    return doc;
}