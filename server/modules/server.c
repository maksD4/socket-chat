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
#include <server/headers/modules/login.h>
#include <bson/bson.h> // temporary

#include "server/headers/data/mongo.h"
#include "server/headers/data/redis.h"
#include "server/headers/data/utils.h"
#include "server/headers/modules/server.h"

pid_t server_pid;
pid_t login_pid;

void signal_kill(int sig_num){
    printf("[%d] Server kill signal(%d)!\n", getpid(), sig_num);
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

        signal(10, signal_kill);

        login_pid = create_login_process();
        printf("[%d] Server boots up!\n", getpid());
        server();
        
    }
    else{
        server_pid = pid;
        char command[256];
        for(;;){
            bzero(command, 256);
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
    printf("login_pid: %d\nsever_pid: %d\n", login_pid, getpid());
    kill(login_pid, SIGTERM);
    kill(getpid(), SIGTERM);
}

void cleanup_server(){
    printf("Server cleanup!\n");

    redis_cleanup();
    sleep(5);
    mongodb_cleanup();
    
    printf("Server has been safely shut down.\n");
}