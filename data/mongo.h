#ifndef MAP_LOADER_H
#define MAP_LOADER_H
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#pragma once

//#define mongoc_client_t *client;
//#define mongoc_database_t *database;

int mongodb_init();
void mongodb_cleanup();
void mongodb_insert(const char *collection_name, bson_t doc);
void mongodb_print_collection(mongoc_collection_t *collection);
bson_t bson_create_user(int user_id, const char* name, const char* password, int* friends_id, int* chats_id);
bson_t bson_create_user_num(int user_id, const char* name, const char* password, int* friends_id, int num_friends, int* chats_id, int num_chats);
#endif