#ifndef MESSAGE_H
#define MESSAGE_H
#include "lib/constants.h"

typedef struct message
{
    int msg_id;
    int sent_by;
    char message[MESSAGE_MAX_SIZE + 1];
    long long int date;
}message_t;


#endif