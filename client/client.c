#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "ipc.h"

int main(void)
{
    int client_fd;

    printf("Connecting to server...\n");

    client_fd = connect_server();

    if (client_fd < 0)
    {
        return 1;
    }

    printf("Connection successful!\n");
    
    const char *message = "Hello Server";
    send_message(client_fd, message, strlen(message));

    close_connection(client_fd);

    return 0;
}
