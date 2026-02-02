#ifndef REDIS_USER_H
#include "server/utils/models/user.h"
#define REDIS_USER_H

char* get_name(int user_id);
int get_user_id(const char* name);

int redis_user_write(user_t user);
int redis_user_read(char *session_key, user_t *user);
int redis_user_exist(int id);
int redis_user_get_name(int id, char **name);

int redis_user_socket_write(int id, char* session_key, int socket);
int redis_user_socket_read(int id); // returns socket, if it fails then -1
int redis_user_socket_get_id(int socket); // returns user id, -1 if fails
char* redis_user_socket_get_session(int socket);

int redis_user_online(int id); // setting user_id online
int redis_user_offline(int id); // setting user_id offline
int redis_is_user_online(int id); // 0 success, -1 fail

int redis_check_friend_invite(int id1, int id2);
int redis_add_friend_invite(int id1, int id2);
int redis_remove_friend_invite(int id1, int id2);
int redis_add_friend(int id1, int id2);

int redis_user_cleanup(int id);

int redis_add_chat_to_user(int user_id, int chat_id);
#endif