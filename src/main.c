#include <stdio.h>

#include "ipc.h"

//#define BUFFER_SIZE 4096

int main(void)
{
    int server_fd;
    int client_fd;
    int bytes;
    char buffer[BUFFER_SIZE];

    server_fd = create_server_socket();

    if (server_fd < 0)
        return 1;

    printf("Waiting for client...\n");

    client_fd = accept_client(server_fd);

    if (client_fd < 0)
    {
        close_connection(server_fd);
        return 1;
    }

    bytes = receive_message(client_fd, buffer,sizeof(buffer) - 1);
    if (bytes > 0) {
	    buffer[bytes] = '\0';
	    printf("Received: %s\n", buffer);
    }



    close_connection(client_fd);
    close_connection(server_fd);

    return 0;
}
