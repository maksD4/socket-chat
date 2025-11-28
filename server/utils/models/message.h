#ifndef MESSAGE_H
#define MESSAGE_H
#include "server/utils/constants.h"

typedef struct message
{
    int msg_id;
    int sent_by;
    char message[MESSAGE_MAX_SIZE];
    long long int date;
}message_t;


#endif