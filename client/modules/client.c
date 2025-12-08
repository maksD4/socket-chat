#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client.h"
#include "packets.h"

struct user user;

void set_name(char* name){
    user.name = malloc(strlen(name) + 1);
    if(user.name == NULL){
        perror("malloc failed!");
        exit(1);
    }
    strcpy(user.name, name);
}

void set_session(char *session_key){
    user.session_key = malloc(strlen(session_key) + 1);
    if(user.session_key == NULL){
        perror("malloc failed!");
        exit(1);
    }
    strcpy(user.session_key, session_key);
}

char packet_buffer[1024];
volatile int read_thread_bool = 1;

// break the read thread loop by setting read_thread_bool to 0
void *read_thread_core(void *arg){
    printf("Read thread created\n");
    read_thread_bool = 1;
    int newSocket = *((int *)arg);
    int n;
    while(read_thread_bool){
        n=recv(newSocket , packet_buffer , 2000 , 0);

        if(n<1){
            break;
        }

        char *message = malloc(sizeof(packet_buffer));
        strcpy(message, packet_buffer);

        // process server answer message
        printf("received message: %s\n", message);

        memset(&packet_buffer, 0, sizeof (packet_buffer));
        free(message);
    }
    printf("Read thread is closed!\n");

    pthread_exit(NULL);
}

int send_to_server(char *message){
    if(send(user.connection.clientSocket, message, strlen(message), 0) < 0){
        printf("Send failed");
        return -1;
    }

    return 0;
}

int server_connect(char *name, char *session_key, int port){
    set_name(name);
    set_session(session_key);

    connection_t *c = &user.connection;

    c->clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    c->serverAddr.sin_family = AF_INET;
    c->serverAddr.sin_port = htons(port);
    c->serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(c->serverAddr.sin_zero, '\0', sizeof(c->serverAddr.sin_zero));

    c->addr_size = sizeof(c->serverAddr);

    if(connect(c->clientSocket, (struct sockaddr *) &c->serverAddr, c->addr_size)){
        printf("CANT CONNECT TO SERVER'S LOGIN SOCKET!\n");
        free(user.name);
        free(user.session_key);
        return -1;
    }

    // create read thread
    if(pthread_create(&c->read_thread, NULL, read_thread_core, &c->clientSocket) != 0){
        printf("Read thread failed to create!\n");
        server_disconnect();
        return -1;
    }


    if(send_to_server("First message!\n")){
        printf("Send message failed!\n");
        server_disconnect();
        return -1;
    }
    sleep(1);

    char** msgs = malloc(4 * sizeof(char *));
    msgs[0] = get_friend_add_packet(session_key, "Alice");
    msgs[1] = get_friend_add_packet(session_key, "Adam");
    msgs[2] = get_friend_removal_packet(session_key, "Adam");
    msgs[3] = get_logout_packet(session_key);
    send_to_server(msgs[0]);
    send_to_server(msgs[1]);
    sleep(1);
    send_to_server(msgs[2]);
    usleep(250000);
    send_to_server(msgs[3]);
    free(msgs);

    return 0;
}

void server_disconnect(){

    // [On leave sending data sequence]

    // End read thread
    read_thread_bool = 0;
    pthread_join(user.connection.read_thread, NULL);

    // Disconnect socket
    if(user.connection.clientSocket == 0){
        printf("LOGIN SOCKET CLOSE ERROR!\n");
        return;
    }
    close(user.connection.clientSocket);

    // Free user memory
    free(user.name);
    free(user.session_key);
}