#ifndef UTILS_H
#define UTILS_H
#include <bson/bson.h>
#include "server/utils/constants.h"
#pragma once

int load_user_to_redis(const char* name);

char* get_name(int user_id);
int get_user_id(const char* name);


#endif