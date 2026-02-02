#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk-4.0/gtk/gtk.h>

#include "lib/constants.h"
#include "client/gui/app.h"
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

    const char* ptr = strchr(packet, ';');

    if(!ptr){
        return -1;
    }
    ptr++;

    int friend_count = atoi(ptr);
    if(friend_count == 0){
        return 0;
    }

    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int old_amount = user_data.friend_amount;
    user_data.friend_amount += friend_count;

    char (*new_friends)[NAME_MAX_SIZE + 1] = realloc(
            user_data.friends,
            user_data.friend_amount * sizeof(char[NAME_MAX_SIZE + 1])
            );

    if(new_friends == NULL){
        printf("Realloc failed!\n");
        user_data.friend_amount = old_amount;
        return -1;
    }
    
    user_data.friends = new_friends;

    for(int i = old_amount; i < user_data.friend_amount; i++){
        const char* next_semicolon = strchr(ptr, ';');
        size_t name_len;

        if(next_semicolon){
            name_len = next_semicolon - ptr;
        }
        else{
            name_len = strlen(ptr); // last name
        }

        memcpy(user_data.friends[i], ptr, name_len);
        user_data.friends[i][name_len] = '\0';

        if(next_semicolon){
            ptr = next_semicolon + 1;
        }
        else{
            break;
        }
    }

    return 0;
}

int extract_room_id(const char* packet){
    const char* ptr = strchr(packet, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    return atoi(ptr);
}

int extract_room(const char* packet){
    int packet_size = strlen(packet);

    const char* ptr = strchr(packet, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int chat_id = atoi(ptr);
    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int user_amount = atoi(ptr);
    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int msg_amount = atoi(ptr);

    if(user_data.room_amount == ROOM_MAX){
        printf("You've reached chat limit!\n");
        return 0;
    }
    
    room_data_t* room = &user_data.rooms[user_data.room_amount];
    user_data.room_amount++;
    
    room->id = chat_id;
    room->user_amount = user_amount;
    room->msg_amount = msg_amount;
    room->messages = malloc(msg_amount * sizeof(msg_data_t));

    if(user_amount == 0){
        return 0;
    }

    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    room->users = malloc(room->user_amount * sizeof(char[NAME_MAX_SIZE + 1]));

    for(int i = 0; i < user_amount; i++){
        const char* next_semicolon = strchr(ptr, ';');
        size_t name_len;

        if(next_semicolon){
            name_len = next_semicolon - ptr;
        }
        else{
            name_len = strlen(ptr); // last name
        }

        memcpy(room->users[i], ptr, name_len);
        room->users[i][name_len] = '\0';

        if(next_semicolon){
            ptr = next_semicolon + 1;
        }
        else{
            break;
        }   
    }

    return 0;
}

static const char* next_msg_struct(const char* ptr){
    for (int i = 0; i < 5; i++) {
        ptr = strchr(ptr, ';');
        if (!ptr){
            return NULL;
        }
        ptr++;
    }
    return ptr;
}

int extract_next_msg_struct(const char* packet, msg_data_t* msg_out){
    const char* ptr = packet;

    msg_out->id = atoi(ptr);

    ptr = strchr(packet, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    const char* sender_ptr = strchr(ptr, ';');
    if(!sender_ptr){
        return -1;
    }

    size_t sender_len = strlen(ptr) - strlen(sender_ptr) + 1;
    if(sender_len > NAME_MAX_SIZE){
        sender_len = NAME_MAX_SIZE;
    }
    memcpy(msg_out->sent_by, ptr, sender_len);
    msg_out->sent_by[sender_len - 1] = '\0';

    ptr = sender_ptr + 1;

    msg_out->date = atoll(ptr);
    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int msg_size = atoi(ptr);
    if(msg_size < 1){
        return -1;
    }

    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    if(msg_size > MESSAGE_MAX_SIZE){
        msg_size = MESSAGE_MAX_SIZE;
    }

    memcpy(msg_out->message, ptr, msg_size);
    msg_out->message[msg_size] = '\0';

    return 0;
}

int extract_rmsg(const char* packet){
    const char* ptr = strchr(packet, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    int chat_id = atoi(ptr);
    if(chat_id < 1){
        return -1;
    }

    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    // Find the room with this chat_id
    int room_index = -1;
    for (int i = 0; i < user_data.room_amount; i++) {
        if (user_data.rooms[i].id == chat_id) {
            room_index = i;
            break;
        }
    }
    
    if (room_index == -1) {
        printf("Room with chat_id %d not found\n", chat_id);
        return -1;
    }

    room_data_t* room = &user_data.rooms[room_index];
    
    if(is_request_pending(RESPONSE_LOGIN)){
        int message_amount = 0;
        const char* test_ptr = ptr;
        while(true){
            printf("test: %s\n", test_ptr);
            test_ptr = next_msg_struct(test_ptr);

            if(test_ptr == NULL){
                printf("Failed to count messages!\n");
                return -1;
            }
            message_amount++;

            printf("test2: %s\n", test_ptr);
            printf("len: %d\n", strlen(test_ptr));
            //test_ptr = strchr(test_ptr, ';');
            if(strlen(test_ptr) == 0){
                break;
            }
            printf("test2.5: %s\n", test_ptr);
            //test_ptr++;
            printf("test3: %s\n", test_ptr);
        }

        if(message_amount > room->msg_amount - room->msg_iter){
            message_amount = room->msg_amount - room->msg_iter;
        }

        printf("amount: %d\n", message_amount);
        msg_data_t* new_msgs = malloc(message_amount * sizeof(msg_data_t));
        for(int i = 0; i < message_amount; i++){
            if(extract_next_msg_struct(ptr, &new_msgs[i]) == -1){
                printf("Failed to extract next msg struct: %s\n", ptr);
                free(new_msgs);
                return -1;
            }

            printf("ptR: %s\n", ptr);
            ptr = next_msg_struct(ptr);
            printf("ptR2: %s\n", ptr);
            // ptr = strchr(ptr, ';');
            // if(!ptr){
            //     printf("Unexpected message extraction failure!\n");
            //     break;
            // }
            // ptr++;
        }

        add_messages_data(chat_id, message_amount, new_msgs);
    }
    else{
       msg_data_t msg = {0};
        if(extract_next_msg_struct(ptr, &msg) == -1){
            printf("Failed to extract msg struct!\n");
            return -1;
        }
        
        room->msg_amount++;

        msg_data_t* msgs = realloc(room->messages, room->msg_amount * sizeof(msg_data_t));

        if(!msgs){
            printf("Failed to reallocate messages!\n");
            return -1;
        }
        room->messages = msgs;
        
        room->messages[room->msg_iter] = msg;
        room->msg_iter++; 
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

            char error[4] = {msg[msg_len - 4], msg[msg_len - 3], msg[msg_len - 2], msg[msg_len - 1]};           
            if(!strcmp(error, "fail")){
                signal_response(RESPONSE_SEND_MESSAGE, FALSE);
                return -1;
            }

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

            char* frnd_reply = NULL;
            if(extract_frnd(msg) == -1){
                printf("frnd fail!\n");
                frnd_reply = get_friend_reply_packet(user_data.session_key, "fail");
                send_to_server(frnd_reply);
                free(frnd_reply);
            }
            else{
                printf("frnd success!\n");
                if(user_data.friend_amount == 0){
                    frnd_reply = get_friend_reply_packet(user_data.session_key, "ok");
                    send_to_server(frnd_reply);
                    free(frnd_reply);

                    signal_response(RESPONSE_LOGIN, TRUE);
                    switch_to_chat_window();
                }
                else{
                    if(!is_request_pending(RESPONSE_LOGIN)){
                        refresh_friends_list();
                    }
                    else{
                        frnd_reply = get_friend_reply_packet(user_data.session_key, "ok");
                        send_to_server(frnd_reply);
                        free(frnd_reply);
                    }
                }
            }

            break;
        case MSG_ROOM:
            // room;ok
            if(msg_len < 7){
                return -1;
            }

            if(!check_reply_state(msg)){
                signal_response(RESPONSE_LOGIN, TRUE);
                switch_to_chat_window();
                break;
            }

            char fail[4] = {msg[msg_len - 4], msg[msg_len - 3], msg[msg_len - 2], msg[msg_len - 1]};
            if(!strcmp(fail, "fail") && msg_len == 9){
                free_user_data();
                signal_response(RESPONSE_LOGIN, FALSE);
                return -1;
            }

            int chat_id = extract_room_id(msg);
            if(chat_id == -1){
                free_user_data();
                signal_response(RESPONSE_LOGIN, FALSE);
                return -1;
            }

            char* room_reply;
            //usleep(100000);
            if(extract_room(msg) == -1){
                printf("Room packet extraction failed!\n");
                room_reply = get_room_reply_packet(user_data.session_key, chat_id, "fail");
                send_to_server(room_reply);
                free(room_reply);
            }
            else{
                if(is_request_pending(RESPONSE_LOGIN)){
                    room_reply = get_room_reply_packet(user_data.session_key, chat_id, "ok");
                    send_to_server(room_reply);
                    free(room_reply);
                }
                else{
                    // update chats list
                    g_idle_add((GSourceFunc)refresh_chats_list, NULL);
                }
            }
            
            break;
        case MSG_RMSG:
            // Minimal size of smsg packet
            // smsg;<chat_id>;<msg_id>;<sent_by>;<date>;<msg_size>;<msg>
            if(msg_len < 18){
                return -1;
            }

            char fail2[4] = {msg[msg_len - 4], msg[msg_len - 3], msg[msg_len - 2], msg[msg_len - 1]};
            if(!strcmp(fail2, "fail") && msg_len == 9){
                free_user_data();
                signal_response(RESPONSE_LOGIN, FALSE);
                return -1;
            }

            if(extract_rmsg(msg) == -1){
                printf("rmsg packet extraction failed!\n");
                if(is_request_pending(RESPONSE_LOGIN)){
                    signal_response(RESPONSE_LOGIN, FALSE);
                }
                else if(is_request_pending(RESPONSE_SEND_MESSAGE)){
                    signal_response(RESPONSE_SEND_MESSAGE, FALSE);
                }
                return -1;
            }

            if(is_request_pending(RESPONSE_LOGIN)){
                signal_response(RESPONSE_LOGIN, TRUE);
            }
            else if(is_request_pending(RESPONSE_SEND_MESSAGE)){
                clear_current_message();
                refresh_current_chat();
                signal_response(RESPONSE_SEND_MESSAGE, TRUE);
            }
            else{
                refresh_current_chat();
            }

            break;
        case MSG_NTFI:

            break;
        default:
            printf("ERR >> INVALID MESSAGE ID!\n");
            return -1;
    }

    return 0;
}