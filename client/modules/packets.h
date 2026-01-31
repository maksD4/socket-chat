#ifndef PACKETS_H
#define PACKETS_H

char* get_account_creation_packet(char *name, char *password);
char* get_login_packet(char *name, char *password);

char* get_logout_packet(char* session_key);
char* get_message_packet(char* session_key, int chat_id, char* message);
char* get_room_creation_packet(char* session_key, char* name, char** users, int user_num);
char* get_room_deletion_packet(char* session_key, int chat_id);
char* get_friend_add_packet(char* session_key, char* friend);
char* get_friend_removal_packet(char* session_key, char* friend);

// Replies
char* get_data_request_packet(char* session);
char* get_friend_reply_packet(char* session_key, char* state);
char* get_room_reply_packet(char* session_key, int id, char* state);

#endif