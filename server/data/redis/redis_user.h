#ifndef REDIS_USER_H
#include "server/utils/models/user.h"
#define REDIS_USER_H

int redis_user_write(user_t user);
int redis_user_read(char *session_key, user_t *user);

#endif