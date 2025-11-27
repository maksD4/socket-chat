#include <stdio.h>
#include <stdlib.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "server/data/mongo.h"
#include "server/utils/constants.h"

mongoc_client_t *client;
mongoc_database_t *database;
int highest_user_id;

int mongodb_init(){
    mongoc_init();

    client = mongoc_client_new ("mongodb://root:root@localhost:27017/");
    if(!client){
        fprintf(stderr, "Faile to create client\n");
        return -1;
    }

    database = mongoc_client_get_database(client, DB_NAME);

    bson_t reply;
    bson_error_t error;

    bool client_status = mongoc_client_get_server_status(client, NULL, &reply, &error);

    if(!client_status){
        fprintf(stderr, "Status error: %s\n", error.message);
        mongodb_cleanup();
        return -1;
    }
    mongodb_data_clear();

    // if collection exist then coll is NULL
    mongoc_collection_t *coll = mongoc_database_create_collection(database, "USER", NULL, &error);

    if(coll){
        printf("Collection USER has been created!\n");
    }

    printf("status: %d\n", client_status);

    // print USER collection
    mongodb_print_collection(coll);

    highest_user_id = mongodb_get_highest_id("USER");

    mongoc_collection_destroy(coll);
    bson_destroy(&reply);
    return 0;
}

void mongodb_data_clear(){
    bson_error_t error;
    bson_t *filter = bson_new();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, "USER");

    if(!mongoc_collection_delete_many(collection, filter, NULL, NULL, &error)){
        fprintf(stderr, "Deletion of USER failed: %s\n", error.message);
    }
    mongoc_collection_destroy(collection);
}

void mongodb_cleanup(){
    mongodb_data_clear();

    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    printf("[%d] Mongodb resources has been successfully cleaned up!\n", getpid());
}

void mongodb_insert(const char *collection_name, bson_t document){
    bson_error_t error;
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);

    bson_iter_t iter;
    if(bson_iter_init_find(&iter, &document, "user_id") && BSON_ITER_HOLDS_INT32(&iter)){
        if(bson_iter_int32(&iter) <= highest_user_id){
            printf("[%d] Invalid user id while inserting to db!\n", getpid());
            mongoc_collection_destroy(collection);
            return;
        }
    }

    if(!mongoc_collection_insert_one(collection, &document, NULL, NULL, &error)){
        fprintf(stderr, "Operation insert failed: %s\n", error.message);
    }

    highest_user_id++;

    mongoc_collection_destroy(collection);
}

void mongodb_get_user_doc(const char *collection_name, bson_t *filter, bson_t *opts, const bson_t **doc){
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);

    mongoc_cursor_t *results = mongoc_collection_find_with_opts(collection, filter, opts, NULL);

    mongoc_cursor_next(results, doc);
    /*
    char *str = bson_as_canonical_extended_json(doc, NULL);
    printf("mongodb_get_doc:\n%s\n", str);
    bson_free(str);
    */
    mongoc_cursor_destroy(results);
    mongoc_collection_destroy(collection);
}   

int mongodb_get_highest_id(const char *collection_name){
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);
    int id = 0;

    const bson_t *doc;
    bson_t *filter = bson_new();
    bson_t *opts = BCON_NEW("sort", "{",
                    "user_id", BCON_INT32(-1), "}", 
                    "limit", BCON_INT64(1));

    mongoc_cursor_t *results = mongoc_collection_find_with_opts(collection, filter, opts, NULL);

    if(mongoc_cursor_next(results, &doc)){
        bson_iter_t iter;
        if(bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_INT32(&iter)){
            id = bson_iter_int32(&iter);
        }
    }

    mongoc_cursor_destroy(results);
    bson_destroy(filter);
    bson_destroy(opts);

    return id;
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

bson_t bson_create_user(int user_id, const char* name, const char* password, int* friends_id, int num_friends, int* chats_id, int num_chats){
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
    BSON_APPEND_INT32(&doc, "friends_num", num_friends);

    bson_t chats;
    bson_init(&chats);
    for(int i = 0; i < num_chats; i++){
        snprintf(key, sizeof(key), "%d", i);
        BSON_APPEND_INT32(&chats, key, chats_id[i]);
    }
    BSON_APPEND_ARRAY(&doc, "chats", &chats);
    BSON_APPEND_INT32(&doc, "chats_num", num_chats);

    BSON_APPEND_NOW_UTC(&doc, "last");
    
    return doc;
}

