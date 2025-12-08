#ifndef USER_SERVICE_H
#define USER_SERVICE_H

int account_creation(char* name, char* password);
int account_deletion(char* session_key);
int auth(char* name, char* password);
int friend_add(char* session_key, char* name);
int friend_remove(char* session_key, char* name);
int room_create(char* session_key, int users_num, char** users);
int room_delete(char* session_key, int chat_id);

#endif