#ifndef PACKETS_H
#define PACKETS_H
#include "server/utils/models/message.h"

char* get_session_packet(char* session_key);
char* get_friends_packet(char* session_key);
char* get_room_packet(char* session_key);
char** get_room_message_packet(char* session_key, int chat_id);
char* get_message_packet(message_t msg);
char* get_state_packet(char* packet_id, char* state);
char* get_search_query_packet(char* query);

#endif