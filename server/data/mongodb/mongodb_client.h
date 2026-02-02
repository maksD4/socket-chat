#ifndef MONGODB_CLIENT_H
#define MONGODB_CLIENT_H
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#pragma once

//#define mongoc_client_t *client;
//#define mongoc_database_t *database;

void counter_init(const char* collection_name);
int mongodb_init();
void mongodb_clear_collection(const char *collection_name);
void mongodb_cleanup();
int mongodb_exist(mongoc_collection_t *colletcion, int id);
int mongodb_insert(const char *collection_name, bson_t doc);
int mongodb_update(const char* collection_name, bson_t* filter, bson_t* update);
int mongodb_get_doc(const char *collection_name, bson_t *filter, bson_t *opts, const bson_t **doc);
int get_next_id(const char* collection_name);
void mongodb_print_collection(const char *collection_name);
#endif