#ifndef SERVER_H
#define SERVER_H

//extern pid_t server_pid;

// start server
void start_server();

// function that is forked and works on another process to be easily kill
void server();

// kills server
void kill_server();

// clean ups everything after server shutdown
void cleanup_server();
#endif