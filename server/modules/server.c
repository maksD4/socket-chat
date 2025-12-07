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

#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/utils/models/models_print.h"

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

    int friends[2] = {2, 3};
    int chats[1] = {1};
    user_t bob = create_user(1, "Bob", "password", friends, 2, chats, 1);
    if(mongodb_user_write(bob)){
        printf("[%d][DB] >> DB USER INSERT FAILED!\n", getpid());
    }

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

    room_t chat = create_room(1, usrs, 2, msgs, 2);
    print_room(chat);
    if(mongodb_room_write(chat)){
        printf("[%d][DB] >> DB ROOM INSERT FAILED!\n", getpid());
    }

    if(!mongodb_to_redis("Bob")){
        printf("Bob was successfully transferred from db to redis!\n");
    }
    else{
        printf("Bob's transfer from db to redis has failed!\n");
    }

    if(!mongodb_to_redis("Alice")){
        printf("Alice was successfully transferred from db to redis!\n");
    }
    else{
        printf("Alice's transfer from db to redis has failed!\n");
    }

    

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

    int bob_id = get_user_id("Bob");
    printf("Bob's id: %d\n", bob_id);

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
    sleep(3);
    mongodb_cleanup();

    printf("[%d] Server has been cleaned up!\n", getpid());
}