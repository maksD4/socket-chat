#ifndef REDIS_ROOM_H
#define REDIS_ROOM_H
#include "server/utils/models/room.h"

int redis_room_message_write(int chat_id, message_t messages);
int redis_room_message_next(int user_id, int chat_id, char* message, message_t *msg);

int redis_room_messages_write(int chat_id, message_t *messages, int message_amount);
int redis_room_messages_read(int chat_id, message_t **messages, int message_amount);

int redis_room_write(room_t room);
int redis_room_exist(int chat_id);
int redis_room_belongs_to_user(char* session, int id); // Is user in room
int redis_room_read(int chat_id, room_t *room);

#endif 