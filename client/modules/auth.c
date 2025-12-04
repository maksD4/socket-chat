#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "auth.h"
#include "packets.h"
#include "client.h"

char buffer[1024];
int clientSocket;
struct sockaddr_in serverAddr;
socklen_t addr_size;

int send_message(const char* message, char **received_message){
    if(send(clientSocket , message , strlen(message) , 0) < 0){
        printf("Send failed\n");
        return -1;
    }

    //Read the message from the server into the buffer
    int len = recv(clientSocket, buffer, 1024, 0);
    if(len < 0){
        printf("Receive failed\n");
        return -1;
    }
    //printf("len: %d\n", len);
    buffer[len] = '\0';

    *received_message = malloc(strlen(buffer) + 1);
    if(*received_message == NULL){
        return -1;
    }

    strcpy(*received_message, buffer);

    //Print the received message
    //printf("Data received: %s\n", *received_message);
    return 0;
}

int get_login_socket(){
    return clientSocket;
}

int login_connect(int port){
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    //Configure settings of the server address
    // Address family is Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function 
    serverAddr.sin_port = htons(port);

    //Set IP address
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Connect the socket to the server using the address
    addr_size = sizeof serverAddr;
    if(connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size)){
        printf("CANT CONNECT TO SERVER'S LOGIN SOCKET!\n");
        return -1;
    }
    return 0;
}

int login_disconnect(){
    if(clientSocket == 0){
        printf("LOGIN SOCKET CLOSE ERROR!\n");
        return -1;
    }
    close(clientSocket);
    return 0;
}

int log_in(char* name, char *password){
    if(login_connect(5033)){
        printf("Failed to connect to server!\n");
        return -1;
    }

    char *message = get_packet_logi(name, password);
    char *received_message;

    for(int i = 0; i < 3; i++){
        if(!send_message(message, &received_message)){
            printf("You've successfully logged in!\n");
            break;
        }
        if(i == 2){
            printf("Failed to send credentials to log in!\n");
            if(login_disconnect()){
                printf("Failed to disconnect from server!\n");
            }
            free(message);
            return -1;
        }
        sleep(1);
    }

    if(login_disconnect()){
        printf("Failed to disconnect from server!\n");
    }

    printf("rcv_message: %s\n", received_message);

    if(server_connect(name, received_message, 5033)){
        printf("Faile to make server-session connection!\n");
        return -1;
    }
    server_disconnect();

    free(message);
    free(received_message);
    return 0;
}

int create_account(char *name, char *password){
    // send_message up to 3 times with delay on error
    // then save session and connect make user connection
    if(login_connect(5033)){
        printf("Failed to connect to server!\n");
        return -1; 
    }
    
    char *message = get_packet_accc(name, password);
    char *received_message;
    
    for(int i = 0; i < 3; i++){
        if(!send_message(message, &received_message)){
            printf("You've successfully created %s account!\n", name);
            break;
        }
        if(i == 2){
            printf("Failed to send credentials for account creation!\n");
            if(login_disconnect()){
                printf("Failed to disconnect from server!\n");
            }
            free(message);
            return -1;
        }
        sleep(1);
    }

    if(login_disconnect()){
        printf("Failed to disconnect from server!\n");
    }

    printf("rcv_msg: %s\n", received_message);
    // save user credentials name, received_message
    // make real connection with session

    free(message);
    free(received_message);
    return 0;
}

int connect_to_server(char *session_key){
    

    return 0;
}