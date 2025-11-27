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
#include "server/data/mongo.h"
#include "server/data/redis/redis_client.h"
#include "server/data/utils.h"
#include "server/modules/server.h"

pid_t server_pid;
pid_t login_pid;


void signal_kill(int sig_num){
    printf("[%d] Server is being closed with signal(%d)...\n", getpid(), sig_num);
    cleanup_server();
    sleep(2);
    kill_server();
}


void start_server(){
    pid_t pid = fork();

    if(pid < 0){
        perror("Server process creation has failed!");
    }
    if(pid == 0){
        /*
        int tfd = open("/dev/pts/0", O_WRONLY);
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
        */

        signal(10, signal_kill);

        login_pid = create_login_process();
        printf("[%d] Server boots up...\n", getpid());
        server();
        
    }
    else{
        server_pid = pid;
        char command[256];
        for(;;){
            memset(command, 0, 256);
            scanf("%s", command);
            if(strcmp(command, "kill") == 0){
                kill(server_pid, 10);
                break;
            }
            else{
                printf("Invalid command: %s\n", command);
            }
            fflush(stdin);

        }
    }

}

void server(){
    redis_init();

    if(!mongodb_init()){
        printf("Mongodb has successfully set up!\n");
        
    }

    int friends[1] = {2};
    int chats[1] = {1};
    bson_t user = bson_create_user(1, "Bob", "passwd", friends, 1, chats, 1);
    mongodb_insert("USER", user);
    bson_destroy(&user);

    int friends2[1] = {1};
    bson_t user2 = bson_create_user(2, "Alice", "p4ssw0rd", friends2, 1, chats, 1);
    mongodb_insert("USER", user2);
    bson_destroy(&user2);

    if(!load_user_to_redis("Bob")){
        printf("Bob was successfully transferred from db to redis!\n");
    }
    else{
        printf("Bob's transfer from db to redis has failed!\n");
    }

    for(;;){
        printf("echo!\n");
        sleep(3);
    }
}

void kill_server(){
    printf("[%d] Server has been safely shut down.\n", getpid());
    kill(login_pid, SIGTERM);
    kill(getpid(), SIGTERM);
}

void cleanup_server(){
    redis_cleanup();
    sleep(5);
    mongodb_cleanup();

    printf("[%d] Server has been cleaned up!\n", getpid());
}