#ifndef CONSTANTS_H
#include <stdint.h>
#define CONSTANTS_H

static const uint16_t MAIN_PORT = 10102;
static const uint16_t LOGIN_PORT = 5033;

static const int ID_SIZE = 4;
static const int SESSION_KEY_SIZE = 16;

static const int NAME_MIN_SIZE = 3;
static const int NAME_MAX_SIZE = 16;

static const int PASSWORD_MIN_SIZE = 3;
static const int PASSWORD_MAX_SIZE = 32;

static const int FRIENDS_MAX = 48;
static const int FRIENDS_NUM_SIZE = 2; // Common logarithm of FRIENDS_MAX

static const int ROOM_MAX = 64;
static const int ROOM_USERS_MAX = 16;
static const int ROOM_NUM_SIZE = 2; // Common logarithm of ROOM_MAX

#define MESSAGE_MAX_SIZE 384

static const char DB_NAME[14] = "chat_database";

#endif