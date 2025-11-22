#ifndef REDIS_H
#define REDIS_H
#include "user.h"
#pragma once

int redis_init();
void redis_cleanup();
void redis_write_user(user_t user);
int redis_read_user(char *session, user_t *user);
#endif