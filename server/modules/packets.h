#ifndef PACKETS_H
#define PACKETS_H
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"

int room_packet_transfer(int client_socket, char *session_key);
void send_room_packet_fail(int* client_socket, int* chat_id);
void send_room_packet(int client_socket, char* session_key, int chat_id);
void send_state_packet(int client_socket, char* packet, const char* state);
void send_numbered_state_packet(int client_socket, char* packet, int* number, const char* state);

char* get_session_packet(char* session_key);
char* get_friends_packet(char* session_key);

char* get_room_packet(room_t room);
char* get_message_packet(message_t msg);

char* get_room_message_packet(int chat_id, message_t msg);
char** get_room_message_packets(char* session_key, int chat_id);
char* get_state_packet(char* packet_id, char* state);
char* get_search_query_packet(char* query);

void send_room_packets(int client_socket, char* session_key);
void send_state_packet(int client_socket, char* packet, const char* state);

#endif