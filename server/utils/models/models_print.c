#include <stdio.h>
#include <time.h>
#include <string.h>
#include <time.h>

#include "server/utils/models/models_print.h"
#include "server/utils/models/user.h"

void print_user(user_t user){
    printf("id: %d\n", user.id);
    printf("name: %s\n", user.name);
    printf("Password: %s\n", user.password);

    printf("Friends (%d): ", user.friends_num);
    for(int i = 0; i < user.friends_num; i++){
        printf("%d ", user.friends[i]);
    }

    printf("\nChats (%d): ", user.chats_num);
    for(int i = 0; i < user.chats_num; i++){
        printf("%d ", user.chats[i]);
    }

    printf("\n");
}

void print_message(message_t message){
    printf("{ message id: %d, ", message.msg_id);
    printf("sent_by: %d, ", message.sent_by);
    printf("message: \"%s\", ", message.message);
    
    time_t t = (time_t) message.date;
    struct tm *tm_info = localtime(&t);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("Date: %s}\n", buffer);   
}

void print_room(room_t room){
    printf("id: %d\n", room.id);
    printf("user_amount: %d\n", room.user_amount);
    printf("users: ");
    for(int i = 0; i < room.user_amount; i++){
        printf("%d, ", room.users[i]);
    }
    printf("\nmessage_amount: %d\n", room.message_amount);
    printf("messsages: \n");
    for(int i = 0; i < room.user_amount; i++){
        print_message(room.messages[i]);
    }
}

user_t create_user(int id, char *name, char *password, int *friends, int friends_num, int *chats, int chats_num){
    user_t u;
    u.id = id;
    u.name = name;
    u.password = password;
    u.friends = friends;
    u.friends_num = friends_num;
    u.chats = chats;
    u.chats_num = chats_num;
    return u;
}

message_t create_message(int id, int sent_by, char message[MESSAGE_MAX_SIZE]){
    message_t msg;
    msg.msg_id = id;
    msg.sent_by = sent_by;
    strcpy(msg.message, message);
    msg.date = (long long int) time(NULL);
    return msg;
}

room_t create_room(int id, int *users, int user_amount, message_t *messages, int message_amount){
    room_t room;
    room.id = id;
    room.user_amount = user_amount;
    room.users = users;
    room.message_amount = message_amount;
    room.messages = messages;

    return room;
}