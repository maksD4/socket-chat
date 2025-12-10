#ifndef REDIS_SESSION_H
#define REDIS_SESSION_H

char* session_create();

int redis_session_write(char **session_key, int user_id);
int redis_session_read(const char *session);

int redis_session_delete(const char *session);
int redis_session_exist(const char *session);

#endif