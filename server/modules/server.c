#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>
#include <bson/bson.h> // temporary

#include "server/modules/login.h"
#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/mongodb/mongodb_room.h"
#include "server/data/redis/redis_client.h"
#include "server/data/redis/redis_session.h"
#include "server/data/redis/redis_room.h"
#include "server/data/redis/redis_user.h"
#include "server/data/bridge.h"
#include "server/modules/server.h"
#include "server/modules/user_service.h"

#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/utils/models/models_print.h"
#include "server/handlers/packet_handler.h"
#include "server/modules/packets.h"

pid_t server_pid;
pid_t login_pid;
pid_t redis_pid;


void signal_kill(int sig_num){
    printf("[%d] Process is being closed with signal(%d)...\n", getpid(), sig_num);
    cleanup_process();
    sleep(2);
    kill_process();
}


void start_server(){
    redis_pid = start_redis_server();    
    server_pid = create_server_process();
    login_pid = create_login_process();
    printf("[%d] Server boots up...\n", getpid());

    char command[256];
    for(;;){
        memset(command, 0, 256);
        scanf("%s", command);
        if(strcmp(command, "kill") == 0){
            kill(server_pid, 10);
            kill(login_pid, 10);
            kill(redis_pid, SIGTERM);
            break;
        }
        else{
            printf("Invalid command: %s\n", command);
        }
        fflush(stdin);

    }
    

}

pid_t create_server_process(){
    int pid = fork();
    if(pid < 0){
        perror("Server process error!");
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
        server();
        printf("[%d] Server process has been created!\n", getpid());
    }
    
    return pid;
}

static uint8_t server_packet_buffer[PACKET_MAX_SIZE + 1];

void * server_thread(void *arg){
    printf("New user connected to main server!\n");
    int newSocket = *((int *)arg);
    ssize_t n;
    
    n=recv(newSocket, server_packet_buffer, PACKET_MAX_SIZE, 0);
    printf("Packet received!\n");

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

    memcpy(packet, server_packet_buffer, (size_t) n);
    packet[n] = '\0';

    printf("[Main server thread] Received packet: %s (len: %d, sizeof: %d)\n", server_packet_buffer, (int)strlen(packet), (int)sizeof(packet));

    if(recognize_packet(newSocket, packet, (size_t)n) == -1){
        printf("Didnt recognize packet!\n");
       send_state_packet(newSocket, packet, "fail");
    }
    
    memset(&server_packet_buffer, 0, sizeof (server_packet_buffer));
    free(packet);

    pthread_exit(NULL);
}

void server(){
    redis_init();

    if(!mongodb_init()){
        printf("Mongodb has successfully set up!\n");
    }
    
    /*
    int friends[2] = {2, 3};
    int chats[1] = {1};
    user_t bob = create_user(1, "Bob", "password", friends, 2, chats, 1);

    if(mongodb_user_write(bob)){
        printf("[%d][DB] >> DB USER INSERT FAILED!\n", getpid());
    }
    printf("\n");

    int friends2[1] = {1};
    user_t alice = create_user(2, "Alice", "secret", friends2, 1, chats, 1);
    if(mongodb_user_write(alice)){
        printf("[%d][DB] >> DB USER INSERT FAILED!\n", getpid());
    }

    bob.name = "Robert";
    if(mongodb_user_write(bob)){
        printf("[%d][DB] >> DB USER INSERT FAILED!\n", getpid());
    }

    // Room test
    message_t *msgs = malloc(2 * sizeof(message_t));
    msgs[0] = create_message(1, 1, "Hello Alice!");
    msgs[1] = create_message(2, 2, "Hello Bob!");
    int *usrs = malloc(2 * sizeof(int));
    usrs[0] = 1;
    usrs[1] = 2;

    printf("asd0.5\n");
    room_t chat = create_room(1, usrs, 2, msgs, 2);
    print_room(chat);
    if(mongodb_room_write(chat)){
        printf("[%d][DB] >> DB ROOM INSERT FAILED!\n", getpid());
    }
    printf("asd0.6\n");
    if(!mongodb_to_redis("Bob")){
        printf("Bob was successfully transferred from db to redis!\n");
    }
    else{
        printf("Bob's transfer from db to redis has failed!\n");
    }
    printf("asd0.7\n");
    if(!mongodb_to_redis("Alice")){
        printf("Alice was successfully transferred from db to redis!\n");
    }
    else{
        printf("Alice's transfer from db to redis has failed!\n");
    }

    printf("asd1\n");    

    // Redis session test
    char *test_session;
    if(redis_session_write(&test_session, 3)){
        printf("redis_write fail\n");
    }
    else{
        printf("session:%s userid:%d\n", test_session, redis_session_read(test_session));

        redis_session_delete(test_session);

        printf("session_exist: %d\n", redis_session_exist(test_session));
    }

    printf("Bob's id: %d\n", mongodb_user_get_id("Bob"));
    printf("Alice's id: %d\n", mongodb_user_get_id("Alice"));
    int id_one, id_two;
    printf("auth: %d", auth("Robert", "wrong_password", &id_one));
    printf(", id: %d\n", id_one);
    printf("auth2: %d", auth("Robert", "password", &id_two));
    printf(", id: %d\n", id_two);
    
    for(;;){
        printf("echo!\n");
        sleep(3);
    }
    */

    // login thread
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    //Create the socket. 
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(MAIN_PORT);

    //Set IP address to localhost 
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);


    //Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket 
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket
    if(listen(serverSocket,50)==0){
        printf("[%d] Listening on server port...\n", getpid());
    }
    else{
        printf("Error\n");
    }

    pthread_t thread_id;

    while(1){
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

        if(pthread_create(&thread_id, NULL, server_thread, &newSocket) != 0){
            printf("Failed to create thread\n");
        }

        pthread_detach(thread_id);
        //pthread_join(thread_id,NULL);
    }
}

void kill_process(){
    printf("[%d] Process has been safely shut down.\n", getpid());
    //kill(login_pid, SIGTERM);
    kill(getpid(), SIGTERM);
}

void cleanup_process(){
    redis_cleanup();
    sleep(3);
    mongodb_cleanup();

    printf("[%d] Redis and mongodb have been cleaned up!\n", getpid());
}