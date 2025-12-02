#include <stdio.h>
#include <bson/bson.h>

#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_room.h"
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"
#include "server/utils/constants.h"

bson_t bson_create_message(message_t msg){
    bson_t doc;
    bson_init(&doc);

    BSON_APPEND_INT32(&doc, "msg_id", msg.msg_id);
    BSON_APPEND_INT32(&doc, "sent_by", msg.sent_by);
    BSON_APPEND_UTF8(&doc, "message", msg.message);
    BSON_APPEND_INT64(&doc, "date", msg.date);
    return doc;
}

bson_t* bson_create_room(room_t room){
    bson_t *doc = bson_new();

    BSON_APPEND_INT32(doc, "id", room.id);
    BSON_APPEND_INT32(doc, "user_amount", room.user_amount);
    BSON_APPEND_INT32(doc, "message_amount", room.message_amount);

    bson_t users;
    bson_init(&users);
    char user_id_key[12];
    for(int i = 0; i < room.user_amount; i++){
        snprintf(user_id_key, sizeof(user_id_key), "%d", i);
        BSON_APPEND_INT32(&users, user_id_key, room.users[i]);
    }
    BSON_APPEND_ARRAY(doc, "users", &users);
    //bson_destroy(&users);

    bson_t messages;
    bson_init(&messages);
    char message_key[12];
    for(int i = 0; i < room.message_amount; i++){
        snprintf(message_key, sizeof(message_key), "%d", i);
        bson_t msg = bson_create_message(room.messages[i]);
        BSON_APPEND_DOCUMENT(&messages, message_key, &msg);
        bson_destroy(&msg);
    }
    BSON_APPEND_ARRAY(doc, "messages", &messages);
    //bson_destroy(&messages);

    return doc;
}

int mongodb_room_read(int id, room_t *room){
    const bson_t *doc;
    bson_t *filter = BCON_NEW("id", BCON_INT32(id));
    bson_t *opts = BCON_NEW ("projection", "{",
                    "_id", BCON_BOOL(false),                                                                                                                                                                                                    
                    "id", BCON_BOOL (true),
                    "users", BCON_BOOL (true),
                    "user_amount", BCON_BOOL (true),
                    "messages", BCON_BOOL (true),
                    "message_amount", BCON_BOOL (true),
                    "}");

    if(mongodb_get_doc("ROOM", filter, opts, &doc)){
        printf("[%d][DB] >> COULDNT FIND ROOM WITH %d ID!\n", getpid(), id);
        return -1;
    }

    bson_destroy(filter);
    bson_destroy(opts);

    bson_iter_t iter;

    room->id = id;

    if(bson_iter_init_find(&iter, doc, "user_amount") && BSON_ITER_HOLDS_INT32(&iter)) {
        room->user_amount = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %d's ROOM USER AMOUNT FIND FAILED!\n", getpid(), id);
        return -1;
    }

    if(bson_iter_init_find(&iter, doc, "message_amount") && BSON_ITER_HOLDS_INT32(&iter)){
        room->message_amount = bson_iter_int32(&iter);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGE AMOUNT FIND FAILED!\n", getpid(), id);
        return -1;
    }

    room->users = malloc(room->user_amount * sizeof(int));
    if(bson_iter_init_find(&iter, doc, "users") && BSON_ITER_HOLDS_ARRAY(&iter)){
        uint32_t arr_len;
        const uint8_t *arr_data;
        bson_iter_array(&iter, &arr_len, &arr_data);

        bson_t arr;
        bson_init_static(&arr, arr_data, arr_len);

        bson_iter_t arr_iter;
        bson_iter_init(&arr_iter, &arr);

        int i = 0;
        while (bson_iter_next(&arr_iter) && i < ROOM_MAX) {
            if (BSON_ITER_HOLDS_INT32(&arr_iter)) {
                room->users[i] = bson_iter_int32(&arr_iter);
                i++;
            }
        }
    }
    else{
        fprintf(stderr, "[%d][DB] >> %d's ROOM USERS FIND FAILED!\n", getpid(), id);
        return -1;
    }

    room->messages = malloc(room->message_amount * sizeof(message_t));
    if(bson_iter_init_find(&iter, doc, "messages") && BSON_ITER_HOLDS_ARRAY(&iter)){
        uint32_t arr_len;
        const uint8_t *arr_data;
        bson_iter_array(&iter, &arr_len, &arr_data);

        bson_t *message_arr = bson_new_from_data(arr_data, arr_len);
        bson_iter_t arr_iter;
        bson_iter_init(&arr_iter, message_arr);

        int i = 0;
        while (bson_iter_next(&arr_iter)) {
            if (BSON_ITER_HOLDS_DOCUMENT(&arr_iter)) {
                bson_iter_t msg_iter;
                bson_t msg_doc;

                bson_iter_document(&arr_iter, &arr_len, &arr_data);
                bson_init_static(&msg_doc, arr_data, arr_len);

                message_t *m = &room->messages[i];
                if(bson_iter_init_find(&msg_iter, &msg_doc, "msg_id") && BSON_ITER_HOLDS_INT32(&msg_iter)){
                    m->msg_id = bson_iter_int32(&msg_iter);
                }
                else{
                    fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGE READ FAILED!\n", getpid(), id);
                    bson_destroy(message_arr);
                    return -1;
                }
                
                if(bson_iter_init_find(&msg_iter, &msg_doc, "sent_by") && BSON_ITER_HOLDS_INT32(&msg_iter)){
                    m->sent_by = bson_iter_int32(&msg_iter);
                }
                else{
                    fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGE READ FAILED!\n", getpid(), id);
                    bson_destroy(message_arr);
                    return -1;
                }

                if(bson_iter_init_find(&msg_iter, &msg_doc, "message") && BSON_ITER_HOLDS_UTF8(&msg_iter)){
                    snprintf(m->message, MESSAGE_MAX_SIZE, "%s", bson_iter_utf8(&msg_iter, NULL));
                }
                else{
                    fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGE READ FAILED!\n", getpid(), id);
                    bson_destroy(message_arr);
                    return -1;
                }

                if(bson_iter_init_find(&msg_iter, &msg_doc, "date") && BSON_ITER_HOLDS_INT64(&msg_iter)){
                    m->date = bson_iter_int64(&msg_iter);
                }
                else{
                    fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGE READ FAILED!\n", getpid(), id);
                    bson_destroy(message_arr);
                    return -1;
                }

                i++;
            }
        }
        bson_destroy(message_arr);
    }
    else{
        fprintf(stderr, "[%d][DB] >> %d's ROOM MESSAGES FIND FAILED!\n", getpid(), id);
        return -1;
    }


    return 0;
}


int mongodb_room_write(room_t room){
    bson_t* room_bson = bson_create_room(room);

    if(mongodb_insert("ROOM", *room_bson) == -1){
        printf("[%d][DB] >> MONGODB ROOM:%d INSERT FAILED!\n", getpid(), room.id);
        return -1;
    }

    mongodb_print_collection("ROOM");

    bson_destroy(room_bson);
    return 0;
}

int mongodb_room_any_online(int id, room_t *room){
    for(int i = 0; i < room->user_amount; i++){
        if(id == room->users[i]){
            continue;
        }

        if(!redis_user_exist(room->users[i])){
            return 0;
        }
    }
    return -1;
}