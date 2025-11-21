#ifndef UTILS_H
#define UTILS_H
#include <bson/bson.h>
#include "server/headers/constants.h"
#pragma once

void load_user_to_redis(const char* name);

#endif