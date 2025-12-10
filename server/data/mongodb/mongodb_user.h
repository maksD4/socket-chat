#ifndef MONGODB_USER_H
#define MONGODB_USER_H
#include <bson/bson.h>
#include "server/utils/models/user.h"

bson_t bson_create_user(user_t user);

int mongodb_user_get_name(int id);
int mongodb_user_get_id(char* name);

int mongodb_user_read(char *name, user_t *user);
int mongodb_user_write(user_t user);

#endif