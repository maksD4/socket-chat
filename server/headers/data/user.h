#ifndef USER_H
#define USER_H
#pragma once

typedef struct user
{
    int user_id;

    char* name;
    char* password;

    int *friends;
    int friends_num;

    int *chats;
    int chats_num;
}user_t;

void print_user(user_t user);

#endif