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


#include "server/utils/constants.h"

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
char packet_buffer[512];

void * login_thread(void *arg){
    printf("new thread \n");
    int newSocket = *((int *)arg);
    int n;
    for(;;){
        n=recv(newSocket , packet_buffer , 2000 , 0);

        // TODO: handle login packet (point 4)

        printf("%s\n",packet_buffer);
            if(n<1){
                break;
            }

        char *message = malloc(sizeof(packet_buffer));
        strcpy(message, packet_buffer);

        sleep(1);
        send(newSocket,message,sizeof(message),0);
        memset(&packet_buffer, 0, sizeof (packet_buffer));

    }
    printf("Exit %s's log in thread!\n", "user");

    pthread_exit(NULL);
}
   
pid_t create_login_process(){
    int pid = fork();
    if(pid < 0){
        perror("Login process error!");
    }
    if(pid == 0){
        /*
        int tfd = open("/dev/pts/2", O_WRONLY);
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
        printf("Log in listening pid: %d\n", getpid());

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
        if(listen(loginSocket,50)==0)
            printf("Listening\n");
        else
            printf("Error\n");
            pthread_t thread_id;

            while(1)
            {
                //Accept call creates a new socket for the incoming connection
                addr_size = sizeof serverStorage;
                newSocket = accept(loginSocket, (struct sockaddr *) &serverStorage, &addr_size);

                if( pthread_create(&thread_id, NULL, login_thread, &newSocket) != 0 ){
                    printf("Failed to create thread\n");
                }

                pthread_detach(thread_id);
                //pthread_join(thread_id,NULL);
            }
        

    }
    else{
        return pid;
    }
}