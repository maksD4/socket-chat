#ifndef CLIENT_H
#define CLIENT_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

typedef struct connection
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    pthread_t read_thread;
}connection_t;

typedef struct user
{
    char *name;
    char *session_key;
    connection_t connection;

    int friends_num;
    char **friends;

    int chats_num;
    int *chats;

    int notifications;
    char **notification_msg;
};

extern struct user user;

void set_name(char* name);
void set_session(char *session_key);
void *read_thread_core(void *arg);
int send_to_server(char *message);
int server_connect(char *name, char *session_key, int port);
void server_disconnect();

#endif