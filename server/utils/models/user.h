#ifndef USER_H
#define USER_H
#pragma once

typedef struct user
{
    int id;

    char* name;
    char* password;

    int *friends;
    int friends_num;

    int *chats;
    int chats_num;
}user_t;

#endif