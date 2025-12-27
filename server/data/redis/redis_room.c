#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lib/hiredis/hiredis.h>

#include "server/data/mongodb/mongodb_user.h" // temporarily
#include "server/data/redis/redis_room.h"
#include "server/data/redis/redis_client.h"
#include "server/data/redis/redis_user.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/utils/models/models_print.h"

int redis_room_message_write(int chat_id, message_t message){
    redisContext *c = redis_get();

    redisReply *r;

    // Start RPUSH transaction
    r = redisCommand(c, "MULTI");
    if (r == NULL) {
        fprintf(stderr, "MULTI failed: %s\n", c->errstr);
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "RPUSH room:%d:messages %d", chat_id, message.msg_id);
    if (r == NULL) {
        freeReplyObject(redisCommand(c, "DISCARD"));  // Abort transaction
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "HSET room:%d:msg:%d sent_by %d message %b date %lld",
                        chat_id, message.msg_id, 
                        message.sent_by,
                        message.message, strlen(message.message),
                        message.date);
    if (r == NULL) {
        freeReplyObject(redisCommand(c, "DISCARD"));  // Abort transaction
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "EXEC");
    if (r == NULL) {
        fprintf(stderr, "EXEC failed: %s\n", c->errstr);
        return -1;
    }
    freeReplyObject(r);

    return 0;
}

int redis_room_message_next(int user_id, int chat_id, char* message, message_t *msg){
    redisContext *c;
    redisReply *r;

    int message_id;

    r = redisCommand(c, "MULTI");
    if (r == NULL) {
        fprintf(stderr, "MULTI failed: %s\n", c->errstr);
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "HINCRBY room:%d message_amount 1", chat_id);
    if (r == NULL) {
        freeReplyObject(redisCommand(c, "DISCARD"));  // Abort transaction
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "HGET room:%d message_amount", chat_id);
    if (r == NULL) {
        freeReplyObject(redisCommand(c, "DISCARD"));  // Abort transaction
        return -1;
    }
    freeReplyObject(r);

    r = redisCommand(c, "EXEC");
    if (r == NULL) {
        fprintf(stderr, "EXEC failed: %s\n", c->errstr);
        return -1;
    }

    if (r->type == REDIS_REPLY_NIL) {
        fprintf(stderr, "Transaction aborted (WATCH triggered)\n");
        freeReplyObject(r);
        return -1;
    }
    
    if(r->type == REDIS_REPLY_ARRAY){
        message_id = atoi(r->element[1]->str);
    }
    
    if(message_id > 0){
        *msg = create_message(message_id, user_id, message);
    }
    
    return 0;
}

int redis_room_messages_write(int id, message_t *messages, int message_amount){
    redisContext *c = redis_get();

    redisReply *r;
    for(int i = 0; i < message_amount; i++){
        message_t *message = &messages[i];

        r = redisCommand(c, "RPUSH room:%d:messages %d", id, message->msg_id);
        
        if(r == NULL){
            printf("[%d][REDIS] >> REDIS MESSAGE READ REPLY IS NULL!\n", getpid());
            return -1;
        }

        redisCommand(c, "HSET room:%d:msg:%d sent_by %d message %b date %lld",
                        id, message->msg_id, 
                        message->sent_by,
                        message->message, strlen(message->message),
                        message->date);
    }

    return 0;
}

int redis_room_messages_read(int id, message_t **messages, int message_amount){
    redisContext *c = redis_get();

    *messages = malloc(message_amount * sizeof(message_t));

    redisReply *r = redisCommand(c, "LRANGE room:%d:messages 0 %d", id, message_amount - 1);

    if(r == NULL || r->type != REDIS_REPLY_ARRAY) {
        printf("[REDIS] >> Failed to read message list!\n");
        if(r){
            freeReplyObject(r);
        }
        return -1;
    }

    int count = r->elements;
    if(count > message_amount){
        count = message_amount;
    }

    for(int i = 0; i < count; i++){
        message_t *m = &(*messages)[i];
        m->msg_id = (int) atoi(r->element[i]->str);

        redisReply *h = redisCommand(c, "HMGET room:%d:msg:%d sent_by message date", id, m->msg_id);

        if(h == NULL || h->type != REDIS_REPLY_ARRAY){
            printf("[REDIS] >> FAILED TO READ MESSAGE HAS FOR ID %d\n", m->msg_id);
            if (h){
                freeReplyObject(h);
            } 
            freeReplyObject(r);
            return -1;
        }

        if(h->element[0]->type == REDIS_REPLY_STRING){
            m->sent_by = atoi(h->element[0]->str);
        } 
        else {
            m->sent_by = -1;
        }

        if(h->element[1]->type == REDIS_REPLY_STRING){
            strncpy(m->message, h->element[1]->str, MESSAGE_MAX_SIZE - 1);
            m->message[MESSAGE_MAX_SIZE - 1] = '\0';
        } 
        else {
            m->message[0] = '\0';
        }

        if(h->element[2]->type == REDIS_REPLY_STRING){
            m->date = atoll(h->element[2]->str);
        } 
        else {
            m->date = 0;
        }
        
        printf("[REDIS][%s] >> message: %s\ndate: %lld\n", mongodb_user_get_name(m->sent_by), m->message, m->date);
        freeReplyObject(h);
    }

    freeReplyObject(r);
    return 0;
}

int redis_room_write(room_t room){
    redisContext *c = redis_get();

    if (c == NULL || c->err) {
        return -1;
    }

    if(room.users == NULL || room.messages == NULL){
        printf("[%d][REDIS] >> ROOM IS NULL WHILE WRITING!\n", getpid());
        return -1;
    }

    redisCommand(c, "HSET room:%d user_amount %d message_amount %d", room.id, room.user_amount, room.message_amount);
    
    for(int i = 0; i < room.user_amount; i++){
        redisCommand(c, "RPUSH room:%d:users %d", room.id, room.users[i]);
    }

    redis_room_messages_write(room.id, room.messages, room.message_amount);
    
    return 0;
}

int redis_room_exist(int id){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "EXISTS room:%d", id);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM EXIST CHECK FAILED!\n", getpid());
        return -1;
    }

    int exists = r->integer;
    freeReplyObject(r);

    return exists == 1 ? 0 : -1;
}

int redis_room_belongs_to_user(char* session, int id){
    if(redis_room_exist(id)){
        return -1;
    }

    user_t user;
    if(redis_user_read(session, &user)){
        printf("[%d][REDIS] >> COULDN'T READ USER!\n", getpid());
        
        if(user.chats != NULL){
            free(user.chats);
        }

        if(user.friends != NULL){
            free(user.friends);
        }

        return -1;
    }

    int exist = 0;
    for(int i = 0; i < user.chats_num; i++){
        if(user.chats[i] == id){
            exist = 1;
            break;
        }
    }

    free(user.chats);
    free(user.friends);
    return exist - 1;
}

int redis_room_read(int id, room_t *room){
    redisContext *c = redis_get();

    if (c == NULL || c->err) {
        return -1;
    }

    redisReply *r = redisCommand(c, "HMGET room:%d user_amount message_amount", id);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM READ REPLY IS NULL!\n", getpid());
        return -1;
    }
    //printf("r->elements: %d\n", (int)r->elements);
    if(r->type != REDIS_REPLY_ARRAY || r->elements != 2){
        fprintf(stderr, "HMGET returned unexpected format!\n %s", r->str);
        freeReplyObject(r);
        return -1;
    }

    room->id = id;
    room->user_amount = atoi(r->element[0]->str);
    room->message_amount = atoi(r->element[1]->str);

    room->users = malloc(room->user_amount * sizeof(int));
    room->messages = malloc(room->message_amount * sizeof(message_t));

    r = redisCommand(c, "LRANGE room:%d:users 0 -1", room->id);
    for(int i = 0; i < room->user_amount; i++){
        room->users[i] = atoi(r->element[i]->str);
    }

    freeReplyObject(r);

    if(redis_room_messages_read(room->id, &room->messages, room->message_amount) == -1){
        printf("[%d][REDIS] >> REDIS ROOM MESSAGE READ FAIL!\n", getpid());
        return -1;
    }
    
    return 0;
}