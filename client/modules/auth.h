#ifndef LOGIN_H
#define LOGIN_H

int send_message(const char* message, char **received_message);
int get_login_socket();

int log_in(char* name, char *password);
int create_account(char* name, char *password);

int login_connect(int port);
int login_disconnect();

#endif