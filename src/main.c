#include <stdio.h>
#include <limits.h>

#include "ipc.h"
#include "crypto.h"

int main(int argc, char *argv[])
{
    int server_fd;
    int client_fd;
    int bytes;

    char buffer[BUFFER_SIZE];

    server_fd = create_server_socket();

    if (server_fd < 0) {
        return 1;
    }

    printf("Waiting for client...\n");

    client_fd = accept_client(server_fd);

    if (client_fd < 0) {
        close_connection(server_fd);
        return 1;
    }

    bytes = receive_message(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
        buffer[bytes] = '\0';

        printf("\nReceived signed script.\n");

        char *signature;
        char *script;

        if (extract_signed_data(buffer, &signature, &script) != 0) {
            printf("[ERROR] Failed to parse signed script.\n");

            close_connection(client_fd);
            close_connection(server_fd);

            return 1;
        }


	printf("[INFO] Successfully parsed signed script.\n");
	printf("[INFO] Signature length : %zu bytes\n", strlen(signature));
	printf("[INFO] Script length    : %zu bytes\n", strlen(script));

        if (verify_signed_script("certs", signature, script)) {
            printf("\n[SUCCESS] Signature verification passed.\n");
        }
        else {
            printf("\n[FAILED] Signature verification failed.\n");
        }
    }

    close_connection(client_fd);
    close_connection(server_fd);

    return 0;
}
