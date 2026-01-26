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
#include "client/modules/app/user_app.h"
#include "client/handlers/packet_handler.h"
#include "lib/constants.h"
#include "client/handlers/response_handler.h"
#include "client/modules/app/user_app.h"

char packet_buffer[2 * PACKET_MAX_SIZE + 1];
volatile int read_thread_bool = 1;

// break the read thread loop by setting read_thread_bool to 0
void *read_thread_core(void *arg){
    printf("Read thread created\n");
    read_thread_bool = 1;
    int newSocket = *((int *)arg);
    int n;
    while(read_thread_bool){
        n=recv(newSocket , packet_buffer , 2 * PACKET_MAX_SIZE , 0);

        if(n<1){
            break;
        }

        char *message = malloc(sizeof(packet_buffer));
        strcpy(message, packet_buffer);

        // process server answer message
        printf("Received packet: %s\n", message);

        if(recognize_packet(message) == -1){
            printf("Packet receive failed\n");
        }

        memset(&packet_buffer, 0, sizeof(packet_buffer));
        free(message);
    }
    printf("Read thread is closed!\n");

    pthread_exit(NULL);
}

void disconnect_any_server(){
    // [On leave sending data sequence]

    // End read thread
    read_thread_bool = 0;
    pthread_join(user_data.conn.read_thread, NULL);

    // Disconnect socket
    if(user_data.conn.clientSocket == 0){
        printf("LOGIN SOCKET CLOSE ERROR!\n");
        return;
    }
    close(user_data.conn.clientSocket);
    printf("Disconnecting...\n");
}

int send_to_server(char *message){
    if(send(user_data.conn.clientSocket, message, strlen(message), 0) < 0){
        printf("Send failed");
        return -1;
    }

    return 0;
}

int login_server_connect(int port){
    connection_t *c = &user_data.conn;

    c->clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    c->serverAddr.sin_family = AF_INET;
    c->serverAddr.sin_port = htons(port);
    c->serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(c->serverAddr.sin_zero, '\0', sizeof(c->serverAddr.sin_zero));

    c-> addr_size = sizeof(c->serverAddr);

    if(connect(c->clientSocket, (struct sockaddr *) &c->serverAddr, c->addr_size) == -1){
        printf("CANNOT CONNECT TO LOGIN SERVER!\n");
        return -1;
    }

    if(pthread_create(&c->read_thread, NULL, read_thread_core, &c->clientSocket) != 0){
        printf("Read thread failed to create!\n");
        disconnect_any_server();
        return -1;
    }

    return 0;
}

int main_server_connect(char *name, char *session_key, int port){
    connection_t *c = &user_data.conn;

    c->clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    c->serverAddr.sin_family = AF_INET;
    c->serverAddr.sin_port = htons(port);
    c->serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(c->serverAddr.sin_zero, '\0', sizeof(c->serverAddr.sin_zero));

    c->addr_size = sizeof(c->serverAddr);

    if(connect(c->clientSocket, (struct sockaddr *) &c->serverAddr, c->addr_size)){
        printf("CANT CONNECT TO MAIN SERVER!\n");
        return -1;
    }

    // create read thread
    if(pthread_create(&c->read_thread, NULL, read_thread_core, &c->clientSocket) != 0){
        printf("Read thread failed to create!\n");
        disconnect_any_server();
        return -1;
    }

    /*
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
    */
    
    return 0;
}

int log_in(char* name, char *password){
    if(login_server_connect(LOGIN_PORT)){
        printf("Failed to connect to server!\n");
        return -1;
    }

    char *message = get_login_packet(name, password);

    for(int i = 0; i < 3; i++){
        if(!send_to_server(message)){
            printf("You've successfully sent log in request!\n");
            break;
        }
        if(i == 2){
            printf("Failed to send credentials to log in!\n");

            disconnect_any_server();
            free(message);
            return -1;
        }
        usleep(500000);
    }

    set_username(name);
    free(message);
    return 0;
}

int create_account(char *name, char *password){
    if(login_server_connect(LOGIN_PORT)){
        printf("Failed to connect to server!\n");
        return -1;
    }
    
    char *message = get_account_creation_packet(name, password);
    
    for(int i = 0; i < 3; i++){
        if(!send_to_server(message)){
            printf("You've successfully sent account creation request!\n");
            break;
        }
        if(i == 2){
            printf("Failed to send credentials to create an account!\n");

            disconnect_any_server();
            free(message);
            return -1;
        }
        usleep(500000);
    }

    free(message);
    return 0;
}
