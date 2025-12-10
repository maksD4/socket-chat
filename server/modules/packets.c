#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "server/data/redis/redis_user.h"
#include "server/utils/models/message.h"
#include "server/utils/models/user.h"
#include "lib/constants.h"

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
            friend_name = get_name(user.friends[i]);

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

/*
char* get_room_packet(char* session_key);
char* get_message_packet(message_t msg);
char** get_room_message_packet(char* session_key, int chat_id);
char* get_state_packet(char* packet_id, char* state);
char* get_search_query_packet(char* query);
*/