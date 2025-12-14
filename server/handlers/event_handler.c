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

    if(session_len != SESSION_KEY_SIZE){
        return -1;
    }
    memcpy(session, first + 1, session_len);
    session[session_len] = '\0';
    return 0;
}

int extract_state(const char* packet){
    size_t packet_size = strlen(packet);

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

    if(mongodb_to_redis(name) == -1){
        printf("[%d][DATA] >> LOADING %s USER DATA FROM MONGODB TO REDIS FAILED!\n", getpid(), name);
        send_state_packet(reply_socket, packet, "fail");
        
        // Clean any debris from mongodb_to_redis in redis
        return;
    }

    // send frnd
    char* reply_packet = get_friends_packet(session_key);
    
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