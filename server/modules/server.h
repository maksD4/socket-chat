#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>

//extern pid_t server_pid;

// start server
void start_server();

// Create server process
pid_t create_server_process();

// function that is forked and works on another process to be easily kill
void server();

// kills server
void kill_process();

// executed after signal
void signal_kill(int sig_num);

// clean ups everything after server shutdown
void cleanup_process();
#endif