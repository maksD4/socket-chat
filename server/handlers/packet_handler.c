#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server/handlers/packet_handler.h"
#include "server/handlers/event_handler.h"
#include "lib/constants.h"

// the smallest packet can be accc/logi or data
// accc/logi: packet_id (4 chars), 2 semicolons, NAME_MIN_SIZE, PASSWORD_MIN_SIZE
const static uint8_t LOGIN_PACKET_MIN_SIZE = 4 + 2 + NAME_MIN_SIZE + PASSWORD_MIN_SIZE;

// data: packet_id (4 chars), semicolon, SESSION_KEY_SIZE
const static uint8_t PACKET_MIN_SIZE = 4 + 1 + SESSION_KEY_SIZE;

int recognize_login_packet(int reply_socket, char* packet, size_t packet_size){
    if(packet_size <= LOGIN_PACKET_MIN_SIZE){
        return -1;
    }
    uint32_t id = ID4(packet[0], packet[1], packet[2], packet[3]);

    switch(id){
        case MSG_ACCC:
            on_account_create(reply_socket, packet, packet_size);
            break;
        case MSG_LOGI:
            on_log_in(reply_socket, packet, packet_size);
            break;
        default:
            printf("ERR >> INVALID MESSAGE ID!\n");
            return -1;
    }
    return 0;
}

int recognize_packet(int reply_socket, char* packet, size_t packet_size){
    // packet_id (4 chars), 2 semicolons, NAME_MIN_SIZE, PASSWORD_MIN_SIZE
    if(packet_size <= PACKET_MIN_SIZE){
        return -1;
    }
    uint32_t id = ID4(packet[0], packet[1], packet[2], packet[3]);

    switch(id){
        case MSG_SMSG:
            on_message_request(reply_socket, packet, packet_size);
            break;
        case MSG_FADD:
            
            break;
        case MSG_FRMV:
            
            break;
        case MSG_LOGO:
            on_log_out_request(reply_socket, packet, packet_size);
            break;
        case MSG_RCRE:
            
            break;
        case MSG_RDEL:
            
            break;
        case MSG_FRND:
            on_friend_request(reply_socket, packet, packet_size);
            break;
        case MSG_ROOM:
            on_room_request(reply_socket, packet, packet_size);
            break;
        case MSG_DATA:
            on_data_request(reply_socket, packet, packet_size); 
            break;
        default:
            printf("ERR >> INVALID MESSAGE ID!\n");
            return -1;
    }

    return 0;
}