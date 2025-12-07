#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include "modules/auth.h"
#include "lib/constants.h"

int strict_string(char *str, size_t len){
    for(int i = 0; i < len; i++){
        int c = str[i];
        if(c < 48 || (c > 57 && c < 65) || (c > 90 && c < 97) || c > 122){
            return -1;
        }
    }
    return 0;
}

int main(){ 
    int msg_scanf_size;

    char buffer[300];
    for(;;){
        printf("Enter choice(create or login or exit): ");
        scanf("%s", buffer);

        if(!strcmp("create", buffer)){
            memset(&buffer, 0, sizeof(buffer));
            char *name;
            char *password;
            for(;;){
                printf("Type username: ");
                scanf("%s", buffer);
                size_t len = strlen(buffer);
                if(len > NAME_MAX_SIZE || len < NAME_MIN_SIZE){
                    printf("Your username must have between 3 and 20 characters. Try again!\n");
                }
                else{
                    name = malloc(strlen(buffer) + 1);
                    strcpy(name, buffer);
                    if(strict_string(name, strlen(name))){
                        printf("Your username has illegal characters. Try again!\n");
                        free(name);
                    }
                    else{
                        break;
                    }
                }
            }

            memset(&buffer, 0, sizeof(buffer));
            
            for(;;){
                printf("Type password: ");
                scanf("%s", buffer);
                size_t len = strlen(buffer);

                if(len > PASSWORD_MAX_SIZE || len < PASSWORD_MIN_SIZE){
                    printf("Your password must have between 3 and 32 characters. Try again!\n");
                }
                else{
                    password = malloc(strlen(buffer) + 1);
                    strcpy(password, buffer);
                    if(strict_string(password, strlen(password))){
                        printf("Your username has illegal characters. Try again!\n");
                        free(password);
                    }
                    else{
                        break;
                    }
                }
            }
            printf("create_account(%s, %s)\n", name, password);
            
            create_account(name, password);

            free(name);
            free(password);

            break;
        }

        if(!strcmp("login", buffer)){
            memset(&buffer, 0, sizeof(buffer));
            char *name;
            char *password;
            for(;;){
                printf("Type username: ");
                scanf("%s", buffer);
                size_t len = strlen(buffer);
                if(len > 20 || len < 3){
                    printf("Your username must have between 3 and 20 characters. Try again!\n");
                }
                else{
                    name = malloc(strlen(buffer) + 1);
                    strcpy(name, buffer);
                    if(strict_string(name, strlen(name))){
                        printf("Your username has illegal characters. Try again!\n");
                        free(name);
                    }
                    else{
                        break;
                    }
                }
            }

            memset(&buffer, 0, sizeof(buffer));
            
            for(;;){
                printf("Type password: ");
                scanf("%s", buffer);
                size_t len = strlen(buffer);

                if(len > 32 || len < 3){
                    printf("Your password must have between 3 and 32 characters. Try again!\n");
                }
                else{
                    password = malloc(strlen(buffer) + 1);
                    strcpy(password, buffer);
                    if(strict_string(password, strlen(password))){
                        printf("Your username has illegal characters. Try again!\n");
                        free(password);
                    }
                    else{
                        break;
                    }
                }
            }
            printf("log_in(%s, %s)\n", name, password);
            log_in(name, password);

            free(name);
            free(password);

            break;
        }

        if(!strcmp("exit", buffer)){
            printf("Exit!\n");
            break;
        }
        else{
            printf("Invalid command: %s\n", buffer);
        }
        
    }
    /*
    login_connect(5033);

    printf("socket: %d\n", get_login_socket());

    char message[1000];

    for(;;){

        msg_scanf_size=scanf("%s",message);
        char *s;
        s=strstr(message,"exit");
        if(s != NULL)
        {
            login_disconnect();
            printf("Exiting\n");
            break;

        }

        send_message(message);
        memset(&message, 0, sizeof (message));
    }
    */
  return 0;
}