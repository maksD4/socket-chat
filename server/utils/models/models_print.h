#ifndef MODELS_PRINT_H
#define MODELS_PRINT_H
#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/utils/constants.h"

void print_user(user_t user);
void print_message(message_t msg);
void print_room(room_t room);

message_t create_message(int id, int user_id, char message[MESSAGE_MAX_SIZE]);
room_t create_room(int id, int *users, int user_amount, message_t *messages, int message_amount);

#endif
