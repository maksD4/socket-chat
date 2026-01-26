#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "client/modules/app/user_app.h"

/*
user_data_t user_data = {
    .session_key = "",
    .username = "",
    .friend_amount = 0,
    .friends = NULL,
    .room_amount = 0,
    .rooms = {0}
};
*/
user_data_t user_data = (user_data_t){0};
int user_socket = -1;


void set_username(char* username){
    strcpy(user_data.username, username);
}

void set_session(char* session){
    strcpy(user_data.session_key, session);
}

void set_friend_data(int frnd_amount, char** users){
    user_data.friend_amount = frnd_amount;
    user_data.friends = malloc(user_data.friend_amount * sizeof(*user_data.friends));
    for(int i = 0; i < frnd_amount; i++){
        strcpy(user_data.friends[i], users[i]);
    }
}

void add_room_data(int chat_id, int usr_amount, int msg_amount, char** users){
    int i = user_data.room_amount;
    user_data.rooms[i].id = chat_id;
    user_data.rooms[i].user_amount = usr_amount;
    user_data.rooms[i].msg_amount = msg_amount;
    user_data.rooms[i].msg_iter = 0; // iterator
    user_data.rooms[i].new_msgs.new_msg_amount = 0;
    user_data.rooms[i].new_msgs.messages = NULL;
    user_data.rooms[i].messages = malloc(msg_amount * sizeof(msg_data_t));
    user_data.rooms[i].users = malloc(usr_amount * sizeof(*user_data.rooms[i].users));
    for(int j = 0; j < usr_amount; j++){
        strcpy(user_data.rooms[i].users[j], users[j]);
    }
    user_data.room_amount += 1;
}

// Add messages in chunks of packet
void add_messages_data(int id, int msg_amount, msg_data_t* messages){
    room_data_t *room = NULL;
    for(int i = 0; i < user_data.room_amount; i++){
        if(user_data.rooms[i].id == id){
            room = &user_data.rooms[i];
        }
    }

    if(room == NULL){
        return;
    }
    for(int i = 0; i < msg_amount && room->msg_iter < room->msg_amount; i++){
        room->messages[room->msg_iter].id = messages[i].id;
        strcpy(room->messages[room->msg_iter].sent_by, messages[i].sent_by);
        strcpy(room->messages[room->msg_iter].message, messages[i].message);
        room->messages[room->msg_iter].date = messages[i].date;
        room->msg_iter += 1;
    }
}

void free_user_data(){
    free(user_data.friends);

    for(int i = 0; i < user_data.room_amount; i++){
        free(user_data.rooms[i].messages);
        free(user_data.rooms[i].users);
        free(user_data.rooms[i].new_msgs.messages);
    }

    // Reset to default state
    user_data = (user_data_t){0};
}

static void print_message_data(msg_data_t msg_data){
    printf("\t\t(%d)[%lld] %s >> %s\n", msg_data.id, msg_data.date, msg_data.sent_by, msg_data.message);
}

static void print_room_data(room_data_t room){
    printf("\troom_id: %d\n", room.id);
    printf("\t(%d)users: ", room.user_amount);
    for(int i = 0; i < room.user_amount; i++){
        printf("%s, ", room.users[i]);
    }
    printf("\n\t(%d)messages: \n", room.msg_amount);
    for(int i = 0; i < room.msg_amount; i++){
        print_message_data(room.messages[i]);
    }

    printf("\n\t(%d)new_messages: \n", room.new_msgs.new_msg_amount);
    for(int i = 0; i < room.new_msgs.new_msg_amount; i++){
        print_message_data(room.new_msgs.messages[i]);
    }
    printf("\n");
}

void print_user_data(){
    printf("session: %s\nusername: %s\n", user_data.session_key, user_data.username);
    printf("(%d)friends: ", user_data.friend_amount);
    for(int i = 0; i < user_data.friend_amount; i++){
        printf("%s, ", user_data.friends[i]);
    }
    printf("\n(%d)rooms:\n", user_data.room_amount);
    for(int i = 0; i < user_data.room_amount; i++){
        print_room_data(user_data.rooms[i]);
    }
}

msg_data_t create_msg_data(int id, char* sent_by, char* message){
    msg_data_t msg;
    msg.id = id;
    strcpy(msg.sent_by, sent_by);
    strcpy(msg.message, message);
    msg.date = 2;
    return msg;
}
