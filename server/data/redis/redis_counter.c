#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hiredis/hiredis.h>

#include "server/data/redis/redis_client.h"
#include "server/data/redis/redis_counter.h"
#include "lib/constants.h"

int redis_counters_set(char* packet_id, char* session){
    redisContext *c = redis_get();
    redisReply *r = redisCommand(c, "SET %s:%s:counter 0 EX %d", packet_id, session, PACKET_TTL);

    if(r == NULL){
        printf("[%d][REDIS] >> REDIS MESSAGE READ REPLY IS NULL!\n", getpid());
        return -1;
    }

    return 0;
}

int redis_counter_get(char* packet_id, char* session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "EXISTS %s:%s:counter", packet_id, session);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER EXIST CHECK FAILED!\n", getpid());
        return -1;
    }

    if(r->integer == 0){
        printf("[%d][REDIS] >> COUNTER DOESNT EXIST!\n", getpid());
        freeReplyObject(r);
        return 0;
    }

    r = redisCommand(c, "GET %s:%s:counter", packet_id, session);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER GET FAILED!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return atoi(r->str);
}

int redis_counters_del(char* packet_id, char* session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "DEL %s:%s:session", packet_id, session);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER GET FAILED!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counters_increment(char* packet_id, char* session){
    redisContext *c = redis_get();
    
    int counter = redis_counter_get(packet_id, session);
    if(counter == -1){
        printf("[%d][REDIS] >> COUNTER IS -1!\n", getpid());
        return -1;
    }

    if(counter >= PACKET_FAIL_MAX){
        printf("[%d][REDIS] >> COUNTER EXCEEDS MAXIMUM AMOUNT OF FAILED PACKETS!", getpid());
        return -1;
    }

    // Update expire time to packet_ttl 
    redisReply *r = redisCommand(c, "INCR %s:%s:counter", packet_id, session);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER INCR FAILED!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counter_room_ids_del(char* session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "DEL room:%s:ids", session);

    if(r == NULL){
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counter_room_ids_set(char* session, int amount, int* ids){
    redisContext *c = redis_get();

    redisReply *r;

    for(int i = 0; i < amount; i++){
        r = redisCommand(c, "RPUSH room:%s:ids %d", session, ids[i]);

        if(r == NULL){
            printf("[%d][REDIS] >> ROOM ID(%d) SET FAILED!\n", getpid(), ids[i]);
        }
    }

    r = redisCommand(c, "EXPIRE room:%s:ids 600", session);

    if(r == NULL){
        if(redis_counter_room_ids_del(session)){
            printf("[%d][REDIS] >> ROOM IDS DELETION FAILED!\n", getpid());
        }
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counter_room_id_set(char* session, int id){
    redisContext *c = redis_get();

    redisReply *r;

    r = redisCommand(c, "RPUSH room:%s:ids %d", session, id);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM ID(%d) SET FAILED!\n", getpid(), id);
    }
    

    r = redisCommand(c, "EXPIRE room:%s:ids 600", session);

    if(r == NULL){
        if(redis_counter_room_ids_del(session)){
            printf("[%d][REDIS] >> ROOM IDS DELETION FAILED!\n", getpid());
        }
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counter_room_ids_exist(char* session){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "EXISTS room:%s:ids", session);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM IDS EXIST CHECK FAILED!\n", getpid());
        return -1;
    }

    int exists = r->integer;
    freeReplyObject(r);
    return exists - 1;
}

int redis_counter_room_ids_next(char* session, int old_id){
    redisContext *c = redis_get();

    if(redis_counter_room_ids_exist(session)){
        return -1;
    }

    redisReply *r = redisCommand(c, "LREN room:%s:ids 1 %d", session, old_id);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM IDS GET NEXT VALUE FAILED!\n", getpid());
        return -1;
    }

    if(r->integer == 0){
        printf("[%d][REDIS] >> %d ID DIDN'T EXIST IN ROOM IDS!\n", getpid(), old_id);
    }

    r = redisCommand(c, "LPOP room:%s:ids", session);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM IDS POP FAILED!\n", getpid());
        return -1;
    }

    if(r->type != REDIS_REPLY_STRING){
        printf("[%d][REDIS] >> ROOM IDS NEXT INVAILD REPLY TYPE!\n", getpid());
        freeReplyObject(r);
        return -1;
    }

    int next_id = atoi(r->str);

    r = redisCommand(c, "LLEN room:%s:ids", session);

    if(r == NULL){
        printf("[%d][REDIS] >> ROOM IDS LENGTH CHECK FAIL!\n", getpid());
        return -1;
    }

    if(r->integer == 0){
        if(redis_counter_room_ids_del(session)){
            printf("[%d][REDIS] >> ROOM IDS DELETION FAILED!\n", getpid());
            return -1;
        }
    }

    freeReplyObject(r);
    return next_id;
}

int redis_counter_room_set(char* session, int id){
    redisContext *c = redis_get();
    
    redisReply *r = redisCommand(c, "SET room:%s:%d:counter 0 EX %d", session, id, PACKET_TTL);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER SET FAILED!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return 0;
}

int redis_counter_room_get(char* session, int id){
    redisContext *c = redis_get();

    redisReply *r = redisCommand(c, "EXISTS room:%s:%d:counter", session, id);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER EXIST CHECK FAILED!\n", getpid());
        return -1;
    }

    if(r->integer == 0){
        printf("[%d][REDIS] >> COUNTER DOESNT EXIST!\n", getpid());
        freeReplyObject(r);
        return -1;
    }

    r = redisCommand(c, "GET room:%s:%d:counter", session, id);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER GET FAILED!\n", getpid());
        return -1;
    }

    freeReplyObject(r);
    return atoi(r->str);
}

int redis_counter_room_increment(char* session, int id){
    redisContext *c = redis_get();
    
    int counter = redis_counter_room_get(session, id);
    if(counter == -1){
        printf("[%d][REDIS] >> ROOM COUNTER GET FAIL!\n", getpid());
        return -1;
    }

    if(counter >= PACKET_FAIL_MAX){
        printf("[%d][REDIS] >> ROOM COUTER HAS REACHED MAX VALUE!\n", getpid());
        return -1;
    }

    redisReply *r = redisCommand(c, "INCR room:%s:%d:counter", session, id);

    if(r == NULL){
        printf("[%d][REDIS] >> COUNTER INCR FAILED!\n", getpid());
        return -1;
    }

    return 0;
}