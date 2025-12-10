#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdio.h>

int extract_credentials(const char* packet, char* name, char* password);
int extract_session(const char* packet, char* session);

void on_account_create(int reply_socket, char* packet, size_t packet_size);
void on_log_in(int reply_socket, char* packet, size_t packet_size);
void on_data_request(int reply_socket, char* packet, size_t packet_size);

#endif