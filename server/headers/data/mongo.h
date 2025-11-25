#ifndef MONGO_H
#define MONGO_H
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#pragma once

//#define mongoc_client_t *client;
//#define mongoc_database_t *database;

int mongodb_init();
void mongodb_data_clear();
void mongodb_cleanup();
void mongodb_insert(const char *collection_name, bson_t doc);
void mongodb_get_doc(const char *collection_name, bson_t *filter, bson_t *opts, const bson_t **doc);
void mongodb_print_collection(mongoc_collection_t *collection);
bson_t bson_create_user(int user_id, const char* name, const char* password, int* friends_id, int num_friends, int* chats_id, int num_chats);
#endif