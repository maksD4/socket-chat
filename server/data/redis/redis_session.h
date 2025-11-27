#ifndef REDIS_SESSION_H
#define REDIS_SESSION_H

int redis_session_write(char **session_key);
int redis_session_read(const char *session);

int redis_session_delete(const char *session);
int redis_session_exist(const char *session);

#endif