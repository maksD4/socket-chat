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
#include "server/utils/models/user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "lib/constants.h"

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
    size_t session_len = strlen(first + 1);

    if(session_len < SESSION_KEY_SIZE){
        return -1;
    }
    memcpy(session, first + 1, SESSION_KEY_SIZE);
    session[session_len] = '\0';
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
    if(packet_size < 4 + 2 + SESSION_KEY_SIZE + 2 || packet_size > 4 + 2 + SESSION_KEY_SIZE + 4){
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

    if(redis_user_socket_write(id, reply_socket) == -1){
        printf("[%d][DATA] >> CANNOT SAVE SOCKET IN REDIS!\n", getpid());
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

        // TODO: proceed data request (start room data transfer sequence)
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
    if(extract_room_state(packet, &chat_id, &state)){
        printf("[%d][FRND] >> ROOM ID AND STATE EXTRACTION FAILED!\n", getpid());
        if(chat_id == 0){
            send_state_packet(reply_socket, packet, "fail");
        }
        else{
            send_room_packet_fail(&reply_socket, &chat_id);
        }
        return;
    }
    
    if(state == -1){
        send_room_packet_fail(&reply_socket, &chat_id);
        return;
    }

    char session_key[SESSION_KEY_SIZE + 1];
    if(extract_session(packet, session_key) == -1){
        printf("[%d][FRND] >> SESSION EXTRACTION FAILED!\n", getpid());
        send_room_packet_fail(&reply_socket, &chat_id);
        return;
    }

    send_room_packet(reply_socket, session_key, chat_id);
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
    if(strlen(packet) < 4 + 3 + SESSION_KEY_SIZE + 1 + 1){
        return -1;
    }

    if(packet[4] != ';' || packet[4 + 1 + SESSION_KEY_SIZE] != ';'){
        return -1;
    }

    const char* ptr_msg_size = strchr(packet + 4 + 2 + SESSION_KEY_SIZE, ';');

    if(!ptr_msg_size){
        return -1;
    }

    const char* ptr_msg = strchr(ptr_msg_size + 1, ';');

    if(!ptr_msg){
        return -1;
    }

    size_t msg_size_len = (size_t)(ptr_msg - (ptr_msg_size + 1));

    char* msg_size_str = malloc(msg_size_len + 1);
    memcpy(msg_size_str, ptr_msg_size + 1, msg_size_len);
    msg_size_str[msg_size_len] = '\0';
    *msg_size = atoi(msg_size_str);
    free(msg_size_str);

    if(*msg_size < 1){
        return -1;
    }

    if(*msg_size > MESSAGE_MAX_SIZE - 1){
        *msg_size = MESSAGE_MAX_SIZE - 1;
    }

    *message = malloc((int)(*msg_size) + 1);
    memcpy(*message, ptr_msg + 1, *msg_size);

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

    if(redis_room_belongs_to_user(session_key, chat_id)){
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    
    int user_id = redis_session_read(session_key);
    if(user_id == -1){
        printf("[%d][SMSG] >> USER ID IS -1!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    room_t room;
    if(redis_room_read(chat_id, &room)){
        printf("[%d][SMSG] >> ROOM READ FAILED!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    message_t msg;
    if(redis_room_message_next(user_id, chat_id, message, &msg)){
        printf("[%d][SMSG] >> CANNOT CREATE NEXT MESSAGE!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    if(redis_room_message_write(chat_id, msg)){
        printf("[%d][SMSG] >> CANNOT SAVE MESSAGE IN REDIS!\n", getpid());
        send_numbered_state_packet(reply_socket, packet, &chat_id, "fail");
        return;
    }

    send_numbered_state_packet(reply_socket, packet, &chat_id, "ok");

    // check if any user of chat_id is online
    // if yes then sends packet to them directly
    // if no then now it does nothing
    for(int i = 0; i < room.user_amount; i++){
        if(!redis_is_user_online(room.users[i])){
            int friend_socket = redis_user_socket_read(room.users[i]);
            if(friend_socket == -1){
                printf("[%d][SMSG] >> COULDNT SENT MESSAGE TO ONLINE USER!\n", getpid());
            }
            else{
                char* msg_packet = get_room_message_packet(room.id, msg);
                send(friend_socket, msg_packet, strlen(msg_packet), 0);
            }
        }
    }
}

void on_friend_add_request(int reply_socket, char* packet, size_t packet_size){

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