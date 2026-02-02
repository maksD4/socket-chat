#define _DEFAULT_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server/data/bridge.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/redis/redis_session.h"
#include "server/data/redis/redis_counter.h"
#include "server/data/redis/redis_user.h"
#include "server/data/redis/redis_room.h"
#include "server/handlers/packet_handler.h"
#include "server/modules/packets.h"
#include "server/modules/user_service.h"
#include "server/utils/models/models_print.h"
#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "lib/constants.h"

int extract_friend_name(const char* packet, char* name){
    const char* first = strchr(packet, ';');
    if(!first){
        return -1;
    }

    const char* second = strchr(first + 1, ';');
    if(!second){
        return -1;
    }

    size_t name_len = strlen(second + 1);
    if(name_len > NAME_MAX_SIZE){
        printf("Name is too big while extracting friends name from packet!\n");
        return -1;
    }

    memcpy(name, second + 1, name_len);
    name[name_len] = '\0';

    return 0;
}

int extract_credentials(const char* packet, char* name, char* password){
    const char* first = strchr(packet, ';');
    if(!first){
        return -1;
    }

    const char* second = strchr(first + 1, ';');
    if(!second){
        return -1;
    }

    size_t name_len = (size_t) (second - (first + 1));
    size_t password_len = strlen(second + 1);

    if(name_len < NAME_MIN_SIZE || name_len > NAME_MAX_SIZE || password_len < PASSWORD_MIN_SIZE || password_len > PASSWORD_MAX_SIZE){
        return - 1;
    }

    memcpy(name, first + 1, name_len);
    name[name_len] = '\0';

    memcpy(password, second + 1, password_len);
    password[password_len] = '\0';
    return 0;
}

int extract_session(const char* packet, char* session){
    const char* first = strchr(packet, ';');
    if(!first){
        return -1;
    }
    
    const char* second = strchr(first + 1, ';');
    size_t session_len;
    
    if(second){
        // Session is between two semicolons
        session_len = second - (first + 1);
    } 
    else{
        // Session is at the end (no second semicolon)
        session_len = strlen(first + 1);
    }
    
    if(session_len != SESSION_KEY_SIZE){
        return -1;
    }
    
    memcpy(session, first + 1, SESSION_KEY_SIZE);
    session[SESSION_KEY_SIZE] = '\0';
    return 0;
}

int extract_room_state(const char* packet, int* id_int, int* state_int){
    int packet_size = strlen(packet);
    *state_int = -1;

    // packet_id, semicolons, session, min_id (one digit), min_state
    if(packet_size < 4 + 3 + SESSION_KEY_SIZE + 1 + 2){
        return -1;
    }

    const char* state = strrchr(packet, ';');

    if(!state){
        return -1;
    }

    size_t len = packet_size - strlen(state) + 1;
    char* temp_packet = malloc(len);
    memcpy(temp_packet, packet, len - 1);
    temp_packet[len - 1] = '\0';

    const char* id = strrchr(temp_packet, ';');

    if(!id){
        return -1;
    }

    size_t id_len = strlen(id + 1);
    char* id_str = malloc(id_len);
    memcpy(id_str, id + 1, id_len);
    *id_int = atoi(id_str);

    free(id_str);
    free(temp_packet);

    if(*id_int == 0){
        return -1;
    }

    state++;

    if(!strcmp(state, "ok")){
        *state_int = 0;
    }

    return 0;
}

int extract_state(const char* packet){
    int packet_size = strlen(packet);

    // 4 - packet_id, 2 - semicolons, SESSION_KEY_SIZE, 2 - state {ok, fail}
    if(packet_size < 4 + 2 + SESSION_KEY_SIZE + 2 || packet_size > 4 + 2 + SESSION_KEY_SIZE + 5){
        return -1;
    }

    const char* state = strrchr(packet, ';');

    if(!state){
        return -1;
    }

    state++;

    if(!strcmp(state, "ok")){
        return 0;
    }
    
    return -1;
}

void on_account_create(int reply_socket, char* packet, size_t packet_size){
    char name[NAME_MAX_SIZE + 1];
    char password[PASSWORD_MAX_SIZE + 1];

    // Extract name and password from packet
    if(extract_credentials(packet, name, password) == -1){
        printf("[%d][TEMP] >> CREDENTIALS EXTRACTION FAIL!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    if(account_creation(name, password) == -1){
        printf("[%d][TEMP] >> ACCOUNT CREATION FAIL!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    send_state_packet(reply_socket, packet, "ok");
}

void on_log_in(int reply_socket, char* packet, size_t packet_size){
    char name[NAME_MAX_SIZE + 1];
    char password[PASSWORD_MAX_SIZE + 1];

    // Extract name and password from packet
    if(extract_credentials(packet, name, password) == -1){
        printf("[%d][TEMP] >> CREDENTIALS EXTRACTION FAIL!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    int id; // user id
    if(auth(name, password, &id) == -1){
        printf("[%d][AUTH] >> %s USER FAILED TO LOG IN!\n", getpid(), name);
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    // create session
    char* session_key;
    if(redis_session_write(&session_key, id) == -1){
        printf("[%d][AUTH] >> SESSION WRITE FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        free(session_key);
        return;
    }

    // if(redis_user_socket_write(id, session_key, reply_socket) == -1){
    //     printf("[%d][AUTH] >> REDIS USER SOCKET WRITE FAILED!'n", getpid());
    //     send_state_packet(reply_socket, packet, "fail");
    //     free(session_key);
    //     return;
    // }

    if(redis_user_online(id)){
        printf("[%d][AUTH] >> USER ONLINE PROCEDURE FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        free(session_key);
        return;
    }

    // send session back
    char* reply_packet = get_session_packet(session_key);
    send(reply_socket, reply_packet, strlen(reply_packet), 0);

    free(reply_packet);
    free(session_key);
}

void on_data_request(int reply_socket, char* packet, size_t packet_size){
    // load user data from db to redis
    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][DATA] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    int id = redis_session_read(session_key);
    if(id < 1){
        printf("[%d][DATA] >> COULDNT FIND USER'S ID!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }
    char* name = mongodb_user_get_name(id);
    if(!strcmp(name, "Unknown")){
        printf("[%d][DATA] >> COULDNT FIND USER'S NAME!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    if(redis_user_socket_write(id, session_key, reply_socket) == -1){
        printf("[%d][AUTH] >> REDIS USER SOCKET WRITE FAILED!'n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    if(mongodb_to_redis(name) == -1){
        printf("[%d][DATA] >> LOADING %s USER DATA FROM MONGODB TO REDIS FAILED!\n", getpid(), name);
        send_state_packet(reply_socket, packet, "fail");
        
        // Clean any debris from mongodb_to_redis in redis
        return;
    }

    // send frnd
    char* reply_packet = get_friends_packet(session_key);
    printf("data response: %s\n", reply_packet);
    
    if(reply_packet != NULL){
        send(reply_socket, reply_packet, strlen(reply_packet), 0);
    }
    redis_counters_set("frnd", session_key);
    free(reply_packet);
}

void on_friend_request(int reply_socket, char* packet, size_t packet_size){
    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][FRND] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    if(!extract_state(packet)){
        if(redis_counters_del("frnd", session_key) == -1){
            printf("[%d][FRND] >> COUNTERS DELETE FAILED!\n", getpid());
        }

        // Move it to on_data_request after wait 500ms after frnd packet
        // TODO: proceed data request (start room data transfer sequence)
        usleep(500000);
        printf("START ROOM TRANSFER!\n");
        if(room_packet_transfer(reply_socket, session_key)){
            printf("[%d][ROOM] >> ROOM'S PACKET INIT FAILED!\n");
        }
        
        return;
    }

    if(redis_counters_increment("frnd", session_key) == -1){
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    char* reply_packet = get_friends_packet(session_key);
    
    if(reply_packet != NULL){
        send(reply_socket, reply_packet, strlen(reply_packet), 0);
    }
    free(reply_packet);
}

void on_room_request(int reply_socket, char* packet, size_t packet_size){
    int chat_id, state;
    if(extract_room_state(packet, &chat_id, &state) == -1){
        printf("[%d][FRND] >> ROOM ID AND STATE EXTRACTION FAILED!\n", getpid());
        if(chat_id == 0){
            send_state_packet(reply_socket, packet, "fail");
        }
        else{
            send_room_packet_fail(&reply_socket, &chat_id);
        }
        return;
    }
    printf("chat_id: %d, state: %d\n", chat_id, state);

    if(state == -1){
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][FRND] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_room_packet_fail(&reply_socket, &chat_id);
        return;
    }

    // send next room packet
    int next_id = get_next_room(session_key, chat_id);
    printf("next_id: %d\n", next_id);
    if(next_id == -1){
        printf("[%d][FRND] >> CHAT DIDN'T BELONG TO USER!\n", getpid());
        send_room_packet_fail(&reply_socket, &chat_id);
        return;
    }
    else if(next_id == -2){
        send_state_packet(reply_socket, packet, "ok");
    }
    else{
        send_room_packet(reply_socket, session_key, next_id);
    }
}

int extract_third_number(char* packet, int* number){
    // example: <packet_id>;<session>;<number>...
    if(strlen(packet) < 4 + 1 + SESSION_KEY_SIZE + 1 + 1){
        return -1;
    }

    if(packet[4] != ';' || packet[4 + 1 + SESSION_KEY_SIZE] != ';'){
        return -1;
    }

    const char* ptr_num = strchr(packet + 4 + 1 + SESSION_KEY_SIZE, ';');

    if(!ptr_num){
        return -1;
    }

    const char* ptr_temp = strchr(ptr_num + 1, ';');

    size_t len;

    if(!ptr_temp){
        len = strlen(ptr_num + 1); 
    }
    else{
        len = (size_t)(ptr_temp - (ptr_num + 1));
    }

    char* num_str = malloc(len + 1);
    memcpy(num_str, ptr_num + 1, len);
    num_str[len] = '\0';

    *number = atoi(num_str);

    free(num_str);
    return 0;
}

int extract_packet_message(char* packet, int* msg_size, char** message){
    // example smsg packet: smsg;<session>;<char_id>;<msg_size>;<message>
    const char* ptr = packet;
    for(int i = 0; i < 3; i++){
        ptr = strchr(ptr, ';');
        if(!ptr){
            return -1;
        }
        ptr++;
    }

    (*msg_size) = atoi(ptr);

    if(*msg_size > MESSAGE_MAX_SIZE){
        *msg_size = MESSAGE_MAX_SIZE;
    }

    ptr = strchr(ptr, ';');
    if(!ptr){
        return -1;
    }
    ptr++;

    (*message) = malloc(((*msg_size) + 1) * sizeof(char));
    memcpy((*message), ptr, *msg_size);
    (*message)[*msg_size] = '\0';

    return 0;
}

void on_message_request(int reply_socket, char* packet, size_t packet_size){
    int chat_id;
    if(extract_third_number(packet, &chat_id)){
        printf("[%d][SMSG] >> CHAT ID EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }
    
    int msg_size;
    char* message;
    if(extract_packet_message(packet, &msg_size, &message)){
        printf("[%d][SMSG] >> MESSAGE EXTRACTION FAILED!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("(%d)msg >> %s (len: %d)\n", chat_id, message, msg_size);
    
    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][SMSG] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    if(redis_room_exist(chat_id)){
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_room_exist success!\n");

    if(redis_room_belongs_to_user(session_key, chat_id)){
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_room_belongs_to_user success!\n");

    int user_id = redis_session_read(session_key);
    if(user_id == -1){
        printf("[%d][SMSG] >> USER ID IS -1!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_session_read success!\n");

    room_t room;
    if(redis_room_read(chat_id, &room)){
        printf("[%d][SMSG] >> ROOM READ FAILED!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_room_read success!\n");

    message_t msg;
    if(redis_room_message_next(user_id, chat_id, message, &msg)){
        printf("[%d][SMSG] >> CANNOT CREATE NEXT MESSAGE!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_room_message_next success!\n");

    if(redis_room_message_write(chat_id, msg)){
        printf("[%d][SMSG] >> CANNOT SAVE MESSAGE IN REDIS!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }
    printf("redis_room_message_write success!\n");

    //send_numbered_state_packet(reply_socket, packet, &chat_id, "ok");
    char* msg_packet = get_room_message_packet(room.id, msg);
    send(reply_socket, msg_packet, strlen(msg_packet), 0);

    // check if any user of chat_id is online
    // if yes then sends packet to them directly
    // if no then now it does nothing
    printf("room.user_amount: %d\n", room.user_amount);
    for(int i = 0; i < room.user_amount; i++){
        printf("id: %d\n", room.users[i]);
        if(room.users[i] == user_id){
            continue;
        }

        if(!redis_is_user_online(room.users[i])){
            int friend_socket = redis_user_socket_read(room.users[i]);
            if(friend_socket == -1){
                printf("[%d][SMSG] >> COULDNT SENT MESSAGE TO ONLINE USER!\n", getpid());
            }
            else{
                printf("id2: %d\n", room.users[i]);
                send(friend_socket, msg_packet, strlen(msg_packet), 0);
            }
        }
    }
    free(msg_packet);
}

void on_friend_add_request(int reply_socket, char* packet, size_t packet_size){
    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][FADD] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    char friend_name[NAME_MAX_SIZE + 1];
    if(extract_friend_name(packet, friend_name) == -1){
        printf("[%d][FADD] >> FRIEND'S NAME EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    int friend_id = mongodb_user_get_id(friend_name);
    if(friend_id == -1){
        printf("[%d][FADD] >> CANNOT FIND FRIEND'S ID IN MONGODB!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    user_t user;
    if(redis_user_read(session_key, &user) == -1){
        printf("[%d][FADD] >> CANNOT READ USER FROM REDIS!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }
 
    if(user.id == friend_id){
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    // Repeated invite
    if(!redis_check_friend_invite(user.id, friend_id)){
        send_state_packet(reply_socket, packet, "ok");
        return;
    }
    

    // Invited user already invited sender
    if(!redis_check_friend_invite(friend_id, user.id)){
        // Make them friends
        if(redis_remove_friend_invite(friend_id, user.id) == -1){
            printf("[%d][FADD] >> COULDN'T REMOVE FRIEND INVITE!\n", getpid());
        }

        // Add them at redis level
        if(redis_add_friend(user.id, friend_id) == -1){
            printf("[%d][FADD] >> COULDN'T ADD FRIEND\n", getpid());
        }

        // Send friend add packet to sender
        int user_socket = redis_user_socket_read(user.id);
        if(user_socket == -1){
            printf("[%d][FADD] >> COULDN'T READ SENDER SOCKET!\n", getpid());
        }
        else{
            char* sender_packet = get_friend_packet(friend_name);
            send(user_socket, sender_packet, strlen(sender_packet), 0);
            free(sender_packet);
        }

        // Create room
        int usrs[2] = {user.id, friend_id};
        int new_chat_id = room_creation(2, usrs);

        if(new_chat_id == -1){
            printf("[%d][FADD] >> NEW CHAT ID IS -1!\n", getpid());
            send_state_packet(reply_socket, packet, "fail");
            free_user(&user);
            return;
        }
        
        room_t room;
        char* room_packet = NULL;
        if(redis_room_read(new_chat_id, &room) == -1){
            printf("[%d][FADD] >> COULDN'T READ REDIS ROOM!\n", getpid());
        }
        else{
            room_packet = get_room_packet(room);
            // swap user_socket with reply_socket
            send(user_socket, room_packet, strlen(room_packet), 0);
        }

        // Add chat to sender at redis level
        if(redis_add_chat_to_user(user.id, new_chat_id) == -1){
            printf("[%d][FADD] >> COULDN'T ADD CHAT TO USER!\n", getpid());
        }

        // Check if receiver is online
        if(!redis_user_exist(friend_id)){
            if(redis_add_friend(friend_id, user.id) == -1){
                printf("[%d][FADD] >> COULDN'T ADD FRIEND\n", getpid());
            }

            // Add chat to receiver at redis level (if is online)
            if(redis_add_chat_to_user(friend_id, new_chat_id) == -1){
                printf("[%d][FADD] >> COULDN'T ADD CHAT TO USER!\n", getpid());
            }

            // Send friend and room packet to receiver
            int friend_socket = redis_user_socket_read(friend_id);
            if(friend_socket == -1){
                printf("[%d][FADD] >> COULDN'T READ FRIEND SOCKET!\n", getpid());
            }
            else{
                char* frnd_packet = get_friend_packet(user.name);

                send(friend_socket, frnd_packet, strlen(frnd_packet), 0);
                free(frnd_packet);
            }

            if(room_packet != NULL){
                usleep(250000);
                send(friend_socket, room_packet, strlen(room_packet), 0);
            }
        }
        else{
            user_t friend;
            if(mongodb_user_read(friend_name, &friend) == -1){
                printf("[%d][FADD] >> MONGODB USER READ FAILED!\n", getpid());
            }
            
            friend.friends_num += 1;
            int* friends_new = realloc(friend.friends, friend.friends_num * sizeof(int));

            if(friends_new == NULL){
                printf("[%d][FADD] >> Memory reallocation failed!\n", getpid());
                friend.friends_num -= 1;
            }
            else{
                friend.friends = friends_new;
                friend.friends[friend.friends_num - 1] = user.id;

                if(mongodb_user_write(friend) == -1){
                    printf("[%d][FADD] >> MONGODB USER WRITE FAILED!\n", getpid());
                }   
            }

            if(mongodb_add_chat_to_user(friend_id, new_chat_id) == -1){
                printf("[%d][FADD] >> DB ADD CHAT TO USER FAILED!\n", getpid());
            }
        }

        if(room_packet != NULL){
            free(room_packet);
        }
    }
    else{
        if(redis_add_friend_invite(user.id, friend_id) == -1){
            printf("[%d][FADD] >> COULDN'T ADD FRIEND TO INVITE LIST!\n", getpid());
            send_state_packet(reply_socket, packet, "fail");
            free_user(&user);
            return;
        }
    }

    send_state_packet(reply_socket, packet, "ok");
    free_user(&user);
}

void on_friend_removal_request(int reply_socket, char* packet, size_t packet_size){

}

void on_log_out_request(int reply_socket, char* packet, size_t packet_size){
    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][LOGO] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    if(redis_to_mongodb(session_key)){
        printf("[%d][LOGO] >> REDIS TO MONGODB ERROR!\n", getpid());
        send_state_packet(reply_socket, packet, "fail");
        return;
    }

    send_state_packet(reply_socket, packet, "ok");
}

void on_room_create_request(int reply_socket, char* packet, size_t packet_size){

}

void on_room_removal_request(int reply_socket, char* packet, size_t packet_size){

}