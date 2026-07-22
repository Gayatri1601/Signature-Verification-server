#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc.h"

int main(int argc, char *argv[])
{
    int client_fd;

    if (argc != 2)
    {
        printf("Usage: %s <signed_script>\n", argv[0]);
        return 1;
    }

    printf("Connecting to server...\n");

    client_fd = connect_server();

    if (client_fd < 0)
    {
        return 1;
    }

    printf("Connection successful!\n");

    printf("Opening signed script: %s\n", argv[1]);

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL)
    {
        perror("Failed to open signed script");
        close_connection(client_fd);
        return 1;
    }

    /* Find file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    /* Allocate memory */
    char *buffer = malloc(file_size + 1);

    if (buffer == NULL)
    {
        printf("Memory allocation failed.\n");
        fclose(fp);
        close_connection(client_fd);
        return 1;
    }

    /* Read entire file */
    size_t bytes_read = fread(buffer, 1, file_size, fp);

    buffer[bytes_read] = '\0';

    fclose(fp);

    printf("Sending signed script to server...\n");

    if (send_message(client_fd, buffer, bytes_read) < 0)
    {
        printf("Failed to send signed script.\n");

        free(buffer);
        close_connection(client_fd);

        return 1;
    }

    printf("Signed script sent successfully.\n");

    free(buffer);

    close_connection(client_fd);

    return 0;
}
