#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H
#include "server/utils/models/user.h"
#include <lib/hiredis/hiredis.h>
#pragma once

int redis_init();
void redis_cleanup();
redisContext* redis_get();

void redis_write_user(user_t user);
int redis_read_user(char *session, user_t *user);
#endif