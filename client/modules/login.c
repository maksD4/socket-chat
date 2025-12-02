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
#include "login.h"

char buffer[1024];
int clientSocket;
struct sockaddr_in serverAddr;
socklen_t addr_size;

int send_message(const char* message){
    if(send(clientSocket , message , strlen(message) , 0) < 0){
        printf("Send failed\n");
    }

    //Read the message from the server into the buffer
    if(recv(clientSocket, buffer, 1024, 0) < 0){
        printf("Receive failed\n");
    }
    //Print the received message
    printf("Data received: %s\n",buffer);
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
}

int login_disconnect(){
    if(clientSocket == 0){
        printf("LOGIN SOCKET CLOSE ERROR!\n");
        return -1;
    }
    close(clientSocket);
    return 0;
}