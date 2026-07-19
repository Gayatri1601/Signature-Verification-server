#ifndef IPC_H
#define IPC_H

#include <stddef.h>

#define SOCKET_PATH "/tmp/signature_server.sock"
#define BUFFER_SIZE 4096

int create_server_socket(void);

int accept_client(int server_fd);

int connect_server(void);

int send_message(int fd, const void *buffer, size_t length);

int receive_message(int fd, void *buffer, size_t length);

void close_connection(int fd);

#endif
