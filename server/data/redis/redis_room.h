#ifndef REDIS_ROOM_H
#define REDIS_ROOM_H
#include "server/utils/models/room.h"

int redis_room_message_write(int chat_id, message_t *messages, int message_amount);
int redis_room_message_read(int chat_id, message_t **messages, int message_amount);

int redis_room_write(room_t room);
int redis_room_read(int chat_id, room_t *room);

#endif 