#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdio.h>

int extract_credentials(const char* packet, char* name, char* password);
int extract_session(const char* packet, char* session);
int extract_state(const char* packet);

void on_account_create(int reply_socket, char* packet, size_t packet_size);
void on_log_in(int reply_socket, char* packet, size_t packet_size);

// Start request for data
void on_data_request(int reply_socket, char* packet, size_t packet_size);

// Friends data request
void on_friend_request(int reply_socket, char* packet, size_t packet_size);

#endif