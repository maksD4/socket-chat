#ifndef LOGIN_H
#define LOGIN_H

int send_message(const char* message);
int get_login_socket();
int login_connect(int port);
int login_disconnect();

#endif