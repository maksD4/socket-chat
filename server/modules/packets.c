#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "server/data/mongodb/mongodb_user.h"
#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_room.h"
#include "server/utils/models/message.h"
#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "lib/constants.h"

void send_room_packets(int client_socket, char* session_key){
    

}

void send_state_packet(int client_socket, char* packet, const char* state){
    size_t len;
    char reply[9];
    memcpy(reply, packet, 4);
    reply[4] = ';';

    if(!strcmp(state, "ok")){
        memcpy(reply + 5, "ok", 2);
        len = 7;
    }
    else{
        memcpy(reply + 5, "fail", 4);
        len = 9;
    }

    send(client_socket, reply, len, 0);
}

/*
ssize_t total = 0;
while (total < 9) {
    ssize_t n = send(client_socket, reply + total, 9 - total, 0);
    if (n <= 0) {
        // handle error / disconnect
        return;
    }
    total += n;
}
*/

char* get_session_packet(char* session_key){
    size_t packet_size = strlen(session_key) + 6;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }
    snprintf(packet, packet_size, "logi;%s", session_key);
    return packet;
}


char* get_friends_packet(char* session_key){
    user_t user;
    if(redis_user_read(session_key, &user)){
        printf("[%d][PACKET] >> REDIS USER READ FAIL!\n", getpid());
        return NULL;
    }

    if(user.friends_num > FRIENDS_MAX){
        printf("[%d][PACKET] >> USER %s's FRIENDS NUMBER EXCEEDING FRIENDS_MAX CONSTANT!\n", getpid(), user.name);
        free(user.friends);
        free(user.chats);
        return NULL;
    }

    size_t packet_size = snprintf(NULL, 0, "%d", user.friends_num) + 6;
    char* packet;

    if(user.friends_num == 0){
        packet = malloc(packet_size);
        snprintf(packet, packet_size, "frnd;0");
        free(user.friends);
        free(user.chats);
        return packet;
    }

    char friends[FRIENDS_MAX * 21];
    memset(friends, 0, sizeof(friends));
    for(int i = 0; i < user.friends_num; i++){
        char *friend_name;
        if(!redis_user_get_name(user.friends[i], &friend_name)){
            strcat(friends, ";");
            strcat(friends, friend_name);
        }
        else{
            friend_name = mongodb_user_get_name(user.friends[i]);

            strcat(friends, ";");
            strcat(friends, friend_name);
        }
    }

    // Concatenating friends name here
    packet_size += strlen(friends);
    packet = malloc(packet_size);
    snprintf(packet, packet_size, "frnd;%d%s", user.friends_num, friends);

    free(user.friends);
    free(user.chats);
    
    return packet;
}

char* get_state_packet(char* packet_id, char* state){
    size_t packet_size;
    char* packet;

    packet_size = strlen(packet_id) + strlen(state) + 2;
    packet = malloc(packet_size);

    snprintf(packet, packet_size, "%s;%s", packet_id, state);

    return packet;
}

char* get_room_packet(room_t room){
    // 4 - packet_id
    size_t packet_size = 4 + snprintf(NULL, 0, ";%d;%d;%d", room.id, room.message_amount, room.user_amount);
    char users[room.user_amount * (NAME_MAX_SIZE + 1)];
    for(int i = 0; i < room.user_amount; i++){
        char* user;
        if(redis_user_get_name(room.users[i], &user) == -1){
            user = mongodb_user_get_name(room.users[i]);
        }
        strcat(users, ";");
        strcat(users, user);
        free(user);
    }

    packet_size += strlen(users);
    char* packet = malloc(packet_size);

    snprintf(packet, packet_size, "room;%d;%d;%d%s", room.id, room.user_amount, room.message_amount, users);

    return packet;
}

char* get_message_packet(message_t msg){
    char* sender;
    if(redis_user_get_name(msg.sent_by, &sender) == -1){
        sender = mongodb_user_get_name(msg.sent_by);
    }

    size_t msg_size = strlen(msg.message);
    // 5 - number of semicolons
    size_t packet_size = 5 + snprintf(NULL, 0, "%d", msg.msg_id) + strlen(sender) + snprintf(NULL, 0, "%lld", msg.date) + snprintf(NULL, 0, "%d", msg_size) + msg_size + 1;
    char* packet = malloc(packet_size);

    snprintf(packet, packet_size, "%d;%s;%lld;%d;%s;", msg.msg_id, sender, msg.date, msg_size, msg.message);

    free(sender);
    return packet;
}

char** get_room_message_packet(room_t room, size_t* counter){
    // 4 - packet id, 1 - semicolon
    size_t prefix_size = snprintf(NULL, 0, "rmsg;%d", room.id) + 1;
    char* prefix = malloc(prefix_size);
    
    snprintf(prefix, prefix_size, "rmsg;%d", room.id);

    *counter = 0;
    int sum = strlen(prefix);
    char** temp_packets = malloc(room.message_amount * sizeof(char *));
    char message_packet[PACKET_MAX_SIZE + 1];
    memset(message_packet, 0, sizeof(message_packet));
    snprintf(message_packet, prefix_size, "%s", prefix);
    
    for(int i = 0; i < room.message_amount; i++){
        char* temp_message_packet = get_message_packet(room.messages[i]);
        size_t message_packet_size = strlen(temp_message_packet);

        if(sum + message_packet_size + 1 >= PACKET_MAX_SIZE){
            temp_packets[*counter] = malloc(strlen(message_packet) + 1);
            strcpy(temp_packets[*counter], message_packet);
            memset(message_packet, 0, sizeof(message_packet));

            snprintf(message_packet, prefix_size + message_packet_size + 1, "%s;%s", prefix, temp_message_packet);
            sum = strlen(prefix) + message_packet_size + 1;
            (*counter)++;
        }
        else{
            sum += message_packet_size + 1;
            strcat(message_packet, ";");
            strcat(message_packet, temp_message_packet);
        }
        free(temp_message_packet);
    }
    free(prefix);
    
    temp_packets[*counter] = malloc(strlen(message_packet) + 1);
    strcpy(temp_packets[*counter], message_packet);
    memset(message_packet, 0, sizeof(message_packet));
    counter++;

    char** packets = malloc(*counter * sizeof(char *));
    for(int i = 0; i < *counter; i++){
        packets[i] = malloc(strlen(temp_packets[i]) + 1);
        strcpy(packets[i], temp_packets[i]);
        free(temp_packets[i]);
    }

    free(temp_packets);
    return packets;
}


/*
char* get_search_query_packet(char* query);
*/