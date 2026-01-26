#ifndef USER_APP_H
#define USER_APP_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "lib/constants.h"

typedef struct connection
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    pthread_t read_thread;
}connection_t;


typedef struct message_app_data{
    int id;
    char sent_by[NAME_MAX_SIZE + 1];
    char message[MESSAGE_MAX_SIZE + 1];
    long long int date;

}msg_data_t;

typedef struct new_message_app_data{
    int new_msg_amount;
    msg_data_t *messages;

}new_msg_data_t;

typedef struct room_app_data{
    int id;
    int user_amount;
    int msg_amount;
    int msg_iter;
    msg_data_t *messages;
    char (*users)[NAME_MAX_SIZE + 1];
    new_msg_data_t new_msgs;

}room_data_t;

typedef struct user_app_data{
    char session_key[SESSION_KEY_SIZE + 1];
    char username[NAME_MAX_SIZE + 1];
    int friend_amount;
    char (*friends)[NAME_MAX_SIZE + 1];
    int room_amount;
    room_data_t rooms[ROOM_MAX];
    connection_t conn;
}user_data_t;

extern user_data_t user_data;
extern int user_socket;

void set_username(char* username);

void set_session(char* session);

void set_friend_data(int frnd_amount, char** users);

void add_room_data(int chat_id, int usr_amount, int msg_amount, char** users);
void add_messages_data(int id, int msg_amount, msg_data_t* messages);

void free_user_data();
void print_user_data();
msg_data_t create_msg_data(int id, char* sent_by, char* message);
#endif