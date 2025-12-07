#ifndef REDIS_USER_H
#include "server/utils/models/user.h"
#define REDIS_USER_H

char* get_name(int user_id);
int get_user_id(const char* name);

int redis_user_write(user_t user);
int redis_user_read(char *session_key, user_t *user);
int redis_user_exist(int id);
int redis_user_get_name(int id, char **name);

#endif