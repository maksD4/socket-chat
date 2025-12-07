#ifndef LOGIN_H
#include <sys/types.h>
#define LOGIN_H

int auth(char* name, char* password);
void * login_thread(void *arg);
pid_t create_login_process();

#endif