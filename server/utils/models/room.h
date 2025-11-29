#ifndef ROOM_H
#define ROOM_H
#include "server/utils/models/message.h"

typedef struct room{
    int chat_id;
    int *users;
    int user_amount;
    message_t *messages;
    int message_amount;
}room_t;

#endif