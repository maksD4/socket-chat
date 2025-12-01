#ifndef MONGODB_ROOM_H
#define MONGODB_ROOM_H
#include <bson/bson.h>
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"

bson_t bson_create_message(message_t message);
bson_t* bson_create_room(room_t room);

int mongodb_room_read(int chat_id, room_t *room);
int mongodb_room_write(room_t room);

#endif