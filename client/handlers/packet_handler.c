#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lib/constants.h"
#include "packet_handler.h"
#include "client/modules/client.h"
#include "client/modules/packets.h"
#include "client/modules/app/user_app.h"
#include "client/handlers/response_handler.h"
#include "client/main.h"

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

int extract_frnd(const char* packet){
    int packet_size = strlen(packet);

    const char* frnd_num = strrchr(packet, ';');

    if(!frnd_num){
        return -1;
    }

    size_t frnd_num_len = packet_size - strlen(frnd_num) + 1;
    char* frnd_num_str = malloc(frnd_num_len);
    memcpy(frnd_num_str, frnd_num + 1, frnd_num_len);

    user_data.friend_amount = atoi(frnd_num_str);
    free(frnd_num_str);

    user_data.friends = malloc(user_data.friend_amount * sizeof(*user_data.friends));

    for(int i = 0; i < user_data.friend_amount; i++){
        const char* friend = strrchr(frnd_num + 1, ';');

        size_t friend_len = packet_size - strlen(friend) + 1;
        char* friend_str = malloc(friend_len);
        memcpy(friend_str, friend + 1, friend_len);

        strcpy(user_data.friends[i], friend_str);
        free(friend_str);
    }

    return 0;
}

int recognize_packet(char* msg){
    int msg_len = strlen(msg);
    // Check if packet has packet_id and semicolon
    if(msg_len < 5 || msg_len > PACKET_MAX_SIZE){
        printf("ERR >> MESSAGE HAS INVALID LENGTH!\n");
        return -1;
    }
    uint32_t id = ID4(msg[0], msg[1], msg[2], msg[3]);

    switch(id){
        case MSG_SMSG:

            break;
        case MSG_ACCC:
            if(!check_reply_state(msg)){
                // Account creation success
                signal_response(RESPONSE_REGISTER, TRUE);
            }
            else{
                // Account creation fail
                signal_response(RESPONSE_REGISTER, FALSE);
            }
            disconnect_any_server();
            break;
        case MSG_FADD:
            if(!check_reply_state(msg)){
                // Friend addition success
                signal_response(RESPONSE_ADD_FRIEND, TRUE);
            }
            else{
                // Friend addition fail
                signal_response(RESPONSE_ADD_FRIEND, FALSE);
            }
            break;
        case MSG_FRMV:
            if(!check_reply_state(msg)){
                // Friend removal success
            }
            else{
                // Friend removal fail

            }
            break;
        case MSG_LOGI:
            if(msg_len < 21){
                // session_key extraction fail
                // if(check_reply_state(msg) == -1){
                //     printf("(%d)packet2: %s\n", msg_len, msg);
                //     char fail[4] = {msg[msg_len - 4], msg[msg_len - 3], msg[msg_len - 2], msg[msg_len - 1]};
                //     if(!strcmp(fail, "fail")){
                //         printf("Invalid password or username!\n");
                //         free_user_data();
                //         signal_response(RESPONSE_LOGIN, FALSE);
                //         return -1;
                //     }
                // }
                signal_response(RESPONSE_LOGIN, FALSE);
                free_user_data();
                return -1;
            }

            char* session = malloc(SESSION_KEY_SIZE + 1);
            
            // Extract session key 
            strncpy(session, msg + 5, SESSION_KEY_SIZE);
            session[SESSION_KEY_SIZE] = '\0';
            set_session(session);
            free(session);

            disconnect_any_server();

            if(main_server_connect(user_data.username, user_data.session_key, MAIN_PORT) == -1){
                printf("CANNOT CONNECT TO MAIN SERVER!\n");
                //signal_response(RESPONSE_LOGIN, FALSE);
                cancel_request(RESPONSE_LOGIN);
            }
            else{
                printf("Successfully connected to main server!\n");
                //switch_to_chat_window();
            }

            char* data_request = get_data_request_packet(user_data.session_key);
            
            // Send data request to server
            if(!send_to_server(data_request)){
                // Data request send success
                printf("Successfully sent data request!\n");
                break;
            }
                
            free(data_request);
            //disconnect_any_server();
            break;
        case MSG_DATA:
            //data;ok
            if(msg_len < 7){
                return -1;
            }

            char* data_response = get_data_request_packet(user_data.session_key);
            // Send data request to server
            for(int i = 0; i < 2; i++){
                if(!send_to_server(data_response)){
                    // Data request send success
                    printf("Successfully sent data request!\n");
                    break;
                }
                usleep(100000);
                if(i == 1){
                    // Data request send fail
                    signal_response(RESPONSE_LOGIN, FALSE);
                    free(data_response);
                    return -1;
                }
            }

            free(data_response);
            //disconnect_any_server();
            break;
        case MSG_LOGO:
            if(!check_reply_state(msg)){
                // Log out success
                signal_response(RESPONSE_LOGOUT, TRUE);
            }
            else{
                // Log out fail
                signal_response(RESPONSE_LOGOUT, FALSE);
            }
            break;
        case MSG_RCRE:
            if(!check_reply_state(msg)){
                // Room creation success
                signal_response(RESPONSE_CREATE_CHAT, TRUE);
            }
            else{
                // Room creation fail
                signal_response(RESPONSE_CREATE_CHAT, FALSE);
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
            //frnd;0
            if(msg_len < 6){
                return -1;
            }

            
            if(check_reply_state(msg) == -1){
                char fail[4] = {msg[msg_len - 4], msg[msg_len - 3], msg[msg_len - 2], msg[msg_len - 1]};
                if(!strcmp(fail, "fail") && msg_len == 9){
                    free_user_data();
                    signal_response(RESPONSE_LOGIN, FALSE);
                    return -1;
                }
            }

            char* frnd_reply;
            if(extract_frnd(msg) == -1){
                printf("frnd fail!\n");
                frnd_reply = get_friend_reply_packet(user_data.session_key, "fail");
            }
            else{
                printf("frnd success!\n");
                frnd_reply = get_friend_reply_packet(user_data.session_key, "ok");
                if(user_data.friend_amount == 0){
                    signal_response(RESPONSE_LOGIN, TRUE);
                    switch_to_chat_window();
                }
            }

            send_to_server(frnd_reply);

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