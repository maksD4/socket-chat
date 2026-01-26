#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "packets.h"
#include "client/modules/app/user_app.h"

// Account creation packet
// accc;<name>;<password>
char* get_account_creation_packet(char *name, char *password){
    size_t packet_size = strlen(name) + strlen(password) + 7;

    char *packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "accc;%s;%s", name, password);
    packet[packet_size - 1] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

// Log in packet
// logi;<name>;<password>
char* get_login_packet(char *name, char *password){
    size_t packet_size = strlen(name) + strlen(password) + 7;

    char *packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "logi;%s;%s", name, password);
    packet[packet_size - 1] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

// Log out packet
// logo;<session>
char* get_logout_packet(char* session_key){
    size_t packet_size = strlen(session_key) + 6;

    char* packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "logo;%s", session_key);
    packet[packet_size - 1] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

// Messaging packet
// smsg;<session>;<chat_id>;<message>
char* get_message_packet(char* session_key, int chat_id, char* message){
    size_t chat_id_size = snprintf(NULL, 0, "%d", chat_id);
    size_t packet_size = strlen(session_key) + chat_id_size + strlen(message) + 8;

    char* packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "smsg;%s;%d;%s", session_key, chat_id, message);
    packet[packet_size - 1] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

// Room creation packet (only for custom rooms number_of_people > 3)
// rcre;<session_key>;<number_of_users>;<creator>;<user1>;<user2>;...
char* get_room_creation_packet(char* session_key, char* name, char** users, int user_num){
    size_t users_size = strlen(name) + user_num + 1;
    for(int i = 0; i < user_num; i++){
        users_size += strlen(users[i]);
    }

    char* users_str = malloc(users_size);
    snprintf(users_str, users_size, "%s", name);

    for(int i = 0; i < user_num; i++){
        strcat(users_str, ";"); // unsafe
        strcat(users_str, users[i]);
    }
    users_str[users_size - 1] = '\0';

    // null terminator size is already included in users_size
    size_t packet_size = strlen(session_key) + snprintf(NULL, 0, "%d", user_num) + users_size + 7;
    char* packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "rcre;%s;%d;%s", session_key, user_num, users_str);
    free(users_str);
    return packet;
}

// Room deletion packet
// rdel;<session_key>;<chat_id>
char* get_room_deletion_packet(char* session_key, int chat_id){
    size_t packet_size = strlen(session_key) + snprintf(NULL, 0, "%d", chat_id) + 7;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }
    snprintf(packet, packet_size, "rdel;%s;%d", session_key, chat_id);
    return packet;
}

// Friend addition packet
// fadd;<session_key>;<name>
char* get_friend_add_packet(char* session_key, char* friend){
    size_t packet_size = strlen(session_key) + strlen(friend) + 7;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "fadd;%s;%s", session_key, friend);
    return packet;
}

// Friend removal packet
// frmv;<session_key>;<friend>
char* get_friend_removal_packet(char* session_key, char* friend){
    size_t packet_size = strlen(session_key) + strlen(friend) + 7;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "frmv;%s;%s", session_key, friend);
    return packet;
}

// Data request
// data;<session_key>
char* get_data_request_packet(char* session){
    size_t packet_size = strlen(session) + 6;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "data;%s", session);
    return packet;
}

// Friend data load reply
// frnd;<session>;<state>
char* get_firend_reply_packet(char* session_key, char* state){ // state = {ok, fail}
    size_t packet_size = strlen(session_key) + strlen(state) + 7;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }
    snprintf(packet, packet_size, "frnd;%s;%s", session_key, state);
    return packet;
}

// Room data load reply
// room;<session>;<id>;<state>
char* get_room_reply_packet(char* session_key, int id, char* state){
    size_t packet_size = strlen(session_key) + snprintf(NULL, 0, "%d", id) + strlen(state) + 8;
    char* packet = malloc(packet_size);

    if(!packet){
        return NULL;
    }
    snprintf(packet, packet_size, "room;%s;%d;%s", session_key, id, state);
    return packet;
}