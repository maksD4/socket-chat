#ifndef REDIS_USER_H
#include "server/utils/models/user.h"
#define REDIS_USER_H

char* get_name(int user_id);
int get_user_id(const char* name);

int redis_user_write(user_t user);
int redis_user_read(char *session_key, user_t *user);
int redis_user_exist(int id);
int redis_user_get_name(int id, char **name);

int redis_user_socket_write(int id, int socket);
int redis_user_socket_read(int id); // returns socket, if it fails then -1

int redis_user_online(int id); // setting user_id online
int redis_user_offline(int id); // setting user_id offline
int redis_is_user_online(int id); // 0 success, -1 fail

#endif