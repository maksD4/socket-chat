#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server/modules/user_service.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/redis/redis_session.h"
#include "server/handlers/packet_handler.h"
#include "server/modules/packets.h"
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
        return -1;
    }

    // send session back
    char* reply_packet = get_session_packet(session_key);
    send(reply_socket, reply_packet, strlen(reply_packet), 0);

    free(reply_packet);
    free(session_key);
}

void on_data_request(int reply_socket, char* packet, size_t packet_size){
    // load user data from db to redis
}