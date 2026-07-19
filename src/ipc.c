#include "ipc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


int create_server_socket(void)
{
    int server_fd;
    struct sockaddr_un address;

    /* Create a Unix Domain Socket */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return -1;
    }

    /* Remove any existing socket file */
    unlink(SOCKET_PATH);

    /* Initialize the address structure */
    memset(&address, 0, sizeof(address));

    address.sun_family = AF_UNIX;

    strncpy(address.sun_path,
            SOCKET_PATH,
            sizeof(address.sun_path) - 1);

    /* Bind the socket to the filesystem path */
    if (bind(server_fd,
             (struct sockaddr *)&address,
             sizeof(address)) == -1)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    printf("Socket created successfully.\n");
    printf("Socket bound to %s\n", SOCKET_PATH);

    if (listen(server_fd, 5) == -1)
    {
	    perror("listen");
	    close(server_fd);
	    return -1;
    }	

    return server_fd;
}


int accept_client(int server_fd)
{
    int client_fd;

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1)
    {
        perror("accept");
        return -1;
    }

    printf("Client connected.\n");

    return client_fd;
}


int connect_server(void)
{
    int client_fd;
    struct sockaddr_un address;

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        perror("socket");
        return -1;
    }

    memset(&address, 0, sizeof(address));

    address.sun_family = AF_UNIX;

    strncpy(address.sun_path,
            SOCKET_PATH,
            sizeof(address.sun_path) - 1);

    if (connect(client_fd,
                (struct sockaddr *)&address,
                sizeof(address)) == -1)
    {
        perror("connect");
        close(client_fd);
        return -1;
    }

    printf("Connected to server.\n");

    return client_fd;
}

int send_message(int fd,
                 const void *buffer,
                 size_t length)
{
    int bytes_sent;

    bytes_sent = send(fd, buffer, length, 0);

    if (bytes_sent == -1)
    {
        perror("send");
        return -1;
    }

    return bytes_sent;
}

int receive_message(int fd,
                    void *buffer,
                    size_t length)
{
    int bytes_received;

    bytes_received = recv(fd, buffer, length, 0);

    if (bytes_received == -1)
    {
        perror("recv");
        return -1;
    }

    return bytes_received;
}

void close_connection(int fd)
{
    close(fd);
}
