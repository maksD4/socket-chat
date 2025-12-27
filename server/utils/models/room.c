#include <stdio.h>
#include <stdlib.h>
#include "server/utils/models/room.h"
#include "server/utils/models/message.h"

void free_room(room_t *room){
    if(room == NULL){
        return;
    }

    if(room->users != NULL){
        free(room->users);
    }

    if(room->messages != NULL){
        free(room->messages);
    }
}