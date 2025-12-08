#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "server/data/mongodb/mongodb_client.h"
#include "lib/constants.h"

mongoc_client_t *client;
mongoc_database_t *database;
int highest_user_id;

void counter_init(const char* collection_name){
    if(!collection_name){
        return;
    }

    // Build counter name: <collection_name>_id
    char counter_name[64];
    snprintf(counter_name, sizeof(counter_name), "%s_id", collection_name);

    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, "COUNTERS");
    if(!collection){
        fprintf(stderr, "Failed to get counters collection.\n");
        return;
    }

    bson_t *filter = BCON_NEW("_id", BCON_UTF8(counter_name));
    bson_t *update = BCON_NEW(
        "$setOnInsert", "{",
            "seq", BCON_INT32(1),
        "}"
    );

    bson_error_t error;
    // Upsert: insert if it doesn't exist
    int success = mongoc_collection_update_one(collection, filter, update,
        mongoc_update_flags_new(MONGOC_UPDATE_UPSERT), NULL, &error);

    if(!success){
        fprintf(stderr, "Failed to initialize counter '%s': %s\n", counter_name, error.message);
    }
    else{
        printf("Counter '%s' initialized successfully (or already exists).\n", counter_name);
    }

    bson_destroy(filter);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
}

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
    mongodb_clear_collection("USER");
    mongodb_clear_collection("ROOM");
    mongodb_clear_collection("COUNTERS");

    // if collection exist then coll is NULL
    mongoc_collection_t *user_col = mongoc_database_create_collection(database, "USER", NULL, &error);
    mongoc_collection_t *room_col = mongoc_database_create_collection(database, "ROOM", NULL, &error);

    if(user_col){
        printf("[%d][DB] >> Collection USER has been created!\n", getpid());
    }
    if(room_col){
        printf("[%d][DB] >> Collection ROOM hasb been created!\n", getpid());
    }

    printf("status: %d\n", client_status);

    // print USER collection
    mongodb_print_collection("USER");

    counter_init("USER");
    counter_init("ROOM");

    mongoc_collection_destroy(user_col);
    mongoc_collection_destroy(room_col);
    bson_destroy(&reply);
    return 0;
}

void mongodb_clear_collection(const char *collection_name){
    bson_error_t error;
    bson_t *filter = bson_new();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);

    if(!mongoc_collection_delete_many(collection, filter, NULL, NULL, &error)){
        fprintf(stderr, "Deletion of %s collection failed: %s\n", collection_name, error.message);
    }
    mongoc_collection_destroy(collection);
}

void mongodb_cleanup(){
    //mongodb_clear_collection("USER");
    //mongodb_clear_collection("ROOM");

    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    printf("[%d] Mongodb resources has been successfully cleaned up!\n", getpid());
}
/*
int mongodb_exist(mongoc_collection_t *coll, int id){
    bson_t *filter = BCON_NEW("id", BCON_INT32(id));

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, filter, NULL, NULL);

    const bson_t *doc;
    int exists = mongoc_cursor_next(cursor, &doc);
    printf("EXISTS: %d\n", exists);

    mongoc_cursor_destroy(cursor);
    bson_destroy(filter);
    return 1;
}
*/
int mongodb_exist(mongoc_collection_t *coll, int id){
    bson_t filter;
    bson_init(&filter);
    BSON_APPEND_INT32(&filter, "id", id);

    //mongoc_collection_t *coll = mongoc_client_get_collection(client, DB_NAME, collection_name);
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, &filter, NULL, NULL);

    const bson_t *doc;
    int exists = mongoc_cursor_next(cursor, &doc);

    //mongoc_collection_destroy(coll);
    mongoc_cursor_destroy(cursor);
    bson_destroy(&filter);
    return exists == 1 ? 0 : -1;
}

int mongodb_insert(const char *collection_name, bson_t document){
    bson_error_t error;
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);

    bson_iter_t iter;
    int id = -1;

    if(bson_iter_init_find(&iter, &document, "id") && BSON_ITER_HOLDS_INT32(&iter)){
        id = bson_iter_int32(&iter);
    }

    if(id == -1){
        printf("[%d][DB] >> INSERT DOCUMENT IS MISSING ID VALUE!\n", getpid());
        mongoc_collection_destroy(collection);
        return -1;
    }

    if(mongodb_exist(collection, id)){
        // Insert if there is no document with certain id
        if(!mongoc_collection_insert_one(collection, &document, NULL, NULL, &error)){
            fprintf(stderr, "[%d][DB] >> OPERATION INSERT FAILED: %s\n", getpid(), error.message);
            mongoc_collection_destroy(collection);
            return -1;
        }
    }
    else{
        // Replace if there is document with certain id
        bson_t filter;
        bson_init(&filter);

        BSON_APPEND_INT32(&filter, "id", id);

        if(!mongoc_collection_replace_one(collection, &filter, &document, NULL, NULL, &error)){
            fprintf(stderr, "[%d][DB] >> REPLACE ONE HAS FAILED: %s\n", error.message);
            bson_destroy(&filter);
            mongoc_collection_destroy(collection);
            return -1;
        }
        bson_destroy(&filter);
    }
    
    mongoc_collection_destroy(collection);
    return 0;
}

int mongodb_get_doc(const char *collection_name, bson_t *filter, bson_t *opts, const bson_t **doc){
    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, collection_name);

    mongoc_cursor_t *results = mongoc_collection_find_with_opts(collection, filter, opts, NULL);

    if(!mongoc_cursor_next(results, doc)){
        mongoc_cursor_destroy(results);
        mongoc_collection_destroy(collection);
        return -1;
    }
    /*
    char *str = bson_as_canonical_extended_json(doc, NULL);
    printf("mongodb_get_doc:\n%s\n", str);
    bson_free(str);
    */
    mongoc_cursor_destroy(results);
    mongoc_collection_destroy(collection);
    return 0;
}   

int get_next_id(const char* collection_name){
    if(!collection_name){
        return -1;
    }

    // Build counter name: <collection_name>_id
    char counter_name[64];
    snprintf(counter_name, sizeof(counter_name), "%s_id", collection_name);

    mongoc_collection_t *collection = mongoc_client_get_collection(client, DB_NAME, "COUNTERS");
    if(!collection){
        fprintf(stderr, "Failed to get COUNTERS collection\n");
        return -1;
    }

    bson_t *filter = BCON_NEW("_id", BCON_UTF8(counter_name));
    bson_t *update = BCON_NEW("$inc", "{", "seq", BCON_INT32(1), "}"); // Increment by one
    bson_t *reply = bson_new();
    bson_error_t error;

    bool success = mongoc_collection_find_and_modify(collection, filter, NULL, update,
                                                    NULL, false, true, true, reply, &error);

    int next_id = -1;
    if(!success){
        fprintf(stderr, "Failed to get next ID: %s\n", error.message);
    } 
    else{
        bson_iter_t iter;
        if(bson_iter_init_find(&iter, reply, "seq") && BSON_ITER_HOLDS_INT32(&iter)){
            next_id = bson_iter_int32(&iter);
        }
    }

    bson_destroy(reply);
    bson_destroy(filter);
    bson_destroy(update);
    mongoc_collection_destroy(collection);

    return next_id;
}


void mongodb_print_collection(const char *collection_name){
    bson_t *query = bson_new();
    mongoc_collection_t *col = mongoc_client_get_collection(client, DB_NAME, collection_name);

    mongoc_cursor_t *results = mongoc_collection_find_with_opts(col, query, NULL, NULL);
    const bson_t *doc;

    while(mongoc_cursor_next(results, &doc)){
        char *str = bson_as_canonical_extended_json(doc, NULL);
        printf("%s\n", str);
        bson_free(str);
    }

    bson_error_t error;
    if(mongoc_cursor_error(results, &error)){
        fprintf(stderr, "Cursor Error: %s\n", error.message);
    }

    mongoc_cursor_destroy(results);
    bson_destroy(query);
}