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
#include <fcntl.h> // for open

#include "server/handlers/packet_handler.h"
#include "server/modules/packets.h"
#include "server/modules/server.h"
#include "server/data/redis/redis_client.h"
#include "server/data/mongodb/mongodb_client.h"
#include "lib/constants.h"

// function in separate fork that waits for login connection and data then proceed
// 1. Client presses button to log in with valid credentials
// 2. Client connects to server
// 3. Client sends credentials
// 4. Server reads credentials and validate them
//  4.1 if data is not valid, server sends back failed login error
// 5. Server sends back session_id
// 6. Client ask for own user data with session key(friends, chats)
// 7. Server checks if any of data is in redis, if not loads them from db to redis
// 8. Server sends (partially) data to client
uint8_t packet_buffer[PACKET_MAX_SIZE + 1];

void * login_thread(void *arg){
    printf("New user connected to login thread!\n");
    int newSocket = *((int *)arg);
    ssize_t n;
    
    n=recv(newSocket, packet_buffer, PACKET_MAX_SIZE, 0);
    printf("Login server received packet!\n");

    // packet_id (4 chars)
    if(n <= 4 || n > PACKET_MAX_SIZE){
        // send back reply
        send(newSocket, "fail", 4, 0);
        pthread_exit(NULL);
    }

    char* packet = malloc((size_t)n + 1);
    if(!packet){
        // send back reply
        pthread_exit(NULL);
    }

    memcpy(packet, packet_buffer, (size_t) n);
    packet[n] = '\0';

    printf("[Login thread] Received packet: %s (len: %d, sizeof: %d)\n", packet_buffer, (int)strlen(packet), (int)sizeof(packet));

    if(recognize_login_packet(newSocket, packet, (size_t)n) == -1){
       send_state_packet(newSocket, packet, "fail");
    }
    
    memset(&packet_buffer, 0, sizeof (packet_buffer));
    free(packet);

    pthread_exit(NULL);
}
   
void login(){
    redis_init();

    if(!mongodb_init()){
        printf("[%d] Mongodb has successfully set up!\n", getpid());
    }

    // login thread
    int loginSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    //Create the socket. 
    loginSocket = socket(PF_INET, SOCK_STREAM, 0);

    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(LOGIN_PORT);

    //Set IP address to localhost 
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);


    //Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket 
    bind(loginSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket
    if(listen(loginSocket,50)==0){
        printf("[%d] Listening on login port...\n", getpid());
    }
    else{
        printf("Error\n");
    }

    pthread_t thread_id;

    while(1){
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        newSocket = accept(loginSocket, (struct sockaddr *) &serverStorage, &addr_size);

        if(pthread_create(&thread_id, NULL, login_thread, &newSocket) != 0){
            printf("Failed to create thread\n");
        }

        pthread_detach(thread_id);
        //pthread_join(thread_id,NULL);
    }
    
}

pid_t create_login_process(){
    int pid = fork();
    if(pid < 0){
        perror("Login process error!");
    }
    if(pid == 0){
        
        int tfd = open("/dev/pts/1", O_WRONLY);
        if (tfd < 0) {
            perror("open pts");
            exit(1);
        }

        // Redirect stdout
        if (dup2(tfd, STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(1);
        }

        close(tfd);
        

        signal(10, signal_kill);
        login();
        printf("[%d] Login process has been created!\n", getpid());
    }
    else{
        return pid;
    }
}