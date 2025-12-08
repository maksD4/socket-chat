#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lib/constants.h"
#include "packet_handler.h"
#include "client/modules/client.h"
#include "client/modules/packets.h"

int check_reply_state(char* message){
    size_t msg_size = strlen(message);
    if(msg_size < 8){
        char ok[2] = {message[msg_size - 2], message[msg_size - 1]};
        if(!strcmp(ok, "ok")){
            return 0;
        }
    }
    return -1;
}

int recognize_packet(char* msg){
    // Check if packet has packet_id and semicolon
    if(strlen(msg) < 5){
        printf("ERR >> MESSAGE TOO SHORT!\n");
        return -1;
    }
    uint32_t id = ID4(msg[0], msg[1], msg[2], msg[3]);

    switch(id){
        case MSG_SMSG:

            break;
        case MSG_ACCC:
            if(!check_reply_state(msg)){
                // Account creation success
            }
            else{
                // Account creation fail
                return -1;
            }
            break;
        case MSG_FADD:
            if(!check_reply_state(msg)){
                // Friend addition success
            }
            else{
                // Friend addition fail
                return -1;
            }
            break;
        case MSG_FRMV:
            if(!check_reply_state(msg)){
                // Friend removal success
            }
            else{
                // Friend removal fail
                return -1;
            }
            break;
        case MSG_LOGI:
            

            if(strlen(msg) < 21){
                // session_key extraction fail
                return -1;
            }

            char* session = malloc(SESSION_KEY_SIZE + 1);
            
            // Extract session key 
            strncpy(session, msg + 5, SESSION_KEY_SIZE);
            session[SESSION_KEY_SIZE] = '\0';
            set_session(session);
            free(session);
            
            // Send data request to server
            for(int i = 0; i < 3; i++){
                char* data_request = get_data_request_packet(user.session_key);
                if(!send_to_server(data_request)){
                    // Data request send success
                    break;
                }
                free(data_request);
                if(i == 2){
                    // Data request send fail
                    return -1;
                }
            }

            break;
        case MSG_LOGO:
            if(!check_reply_state(msg)){
                // Log out success
            }
            else{
                // Log out fail
                return -1;
            }
            break;
        case MSG_RCRE:
            if(!check_reply_state(msg)){
                // Room creation success
            }
            else{
                // Room creation fail
                return -1;
            }
            break;
        case MSG_RDEL:
            if(!check_reply_state(msg)){
                // Room deletion success
            }
            else{
                // Room deletion fail
                return -1;
            }
            break;
        case MSG_FRND:

            break;
        case MSG_ROOM:

            break;
        case MSG_RMSG:

            break;
        case MSG_NTFI:

            break;
        default:
            printf("ERR >> INVALID MESSAGE ID!\n");
            return -1;
    }

    return 0;
}