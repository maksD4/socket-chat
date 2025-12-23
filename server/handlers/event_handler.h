#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdio.h>

int extract_credentials(const char* packet, char* name, char* password);
int extract_session(const char* packet, char* session);
int extract_state(const char* packet);
int extract_third_number(char* packet, int* number);
int extract_packet_message(char* packet, int* msg_size, char** message);

void on_account_create(int reply_socket, char* packet, size_t packet_size);
void on_log_in(int reply_socket, char* packet, size_t packet_size);


// Start request for data
void on_data_request(int reply_socket, char* packet, size_t packet_size);

// Friends data request
void on_friend_request(int reply_socket, char* packet, size_t packet_size);

// Room data request
void on_room_request(int reply_packet, char* packet, size_t packet_size);

// Message send request
void on_message_request(int reply_socket, char* packet, size_t packet_size);

// Friend addition request
void on_friend_add_request(int reply_socket, char* packet, size_t packet_size);

// Friend removal request
void on_friend_removal_request(int reply_socket, char* packet, size_t packet_size);

// Log out request
void on_log_out_request(int reply_socket, char* packet, size_t packet_size);

// Room creation request
void on_room_create_request(int reply_socket, char* packet, size_t packet_size);

// Room removal request
void on_room_removal_request(int reply_socket, char* packet, size_t packet_size);

#endif