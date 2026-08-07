#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>

#include "ipc.h"
#include "crypto.h"

#define MAX_OUTPUT_SIZE 8192

static int execute_script(const char *script,
                          char *output,
                          size_t output_size)
{
    char temp_file[] = "/tmp/scriptXXXXXX";

    int fd = mkstemp(temp_file);
    if (fd == -1)
    {
        perror("mkstemp");
        return -1;
    }

    if (write(fd, script, strlen(script)) == -1)
    {
        perror("write");
        close(fd);
        unlink(temp_file);
        return -1;
    }

    close(fd);

    if (chmod(temp_file, 0700) == -1)
    {
        perror("chmod");
        unlink(temp_file);
        return -1;
    }

    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        unlink(temp_file);
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");

        close(pipefd[0]);
        close(pipefd[1]);
        unlink(temp_file);

        return -1;
    }

    if (pid == 0)
    {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[1]);

        execl("/bin/bash",
              "bash",
              temp_file,
              (char *)NULL);

        perror("execl");
        exit(EXIT_FAILURE);
    }

    close(pipefd[1]);

    ssize_t bytes =
        read(pipefd[0],
             output,
             output_size - 1);

    if (bytes < 0)
    {
        perror("read");
        bytes = 0;
    }

    output[bytes] = '\0';

    close(pipefd[0]);

    waitpid(pid, NULL, 0);

    unlink(temp_file);

    return 0;
}

int main()
{
    int server_fd;

    server_fd = create_server_socket();

    if (server_fd < 0)
    {
        return EXIT_FAILURE;
    }

    signal(SIGCHLD, SIG_IGN);

    printf("Waiting for clients...\n");

    while (1)
    {
        int client_fd;
        int bytes;

        char buffer[BUFFER_SIZE];

        client_fd = accept_client(server_fd);

        if (client_fd < 0)
        {
            continue;
        }

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            close_connection(client_fd);
            continue;
        }

        if (pid == 0)
        {
            /* Child process */

            close_connection(server_fd);

            bytes = receive_message(client_fd,
                                    buffer,
                                    sizeof(buffer) - 1);

            if (bytes <= 0)
            {
                close_connection(client_fd);
                exit(EXIT_FAILURE);
            }

            buffer[bytes] = '\0';

            printf("\n[PID %d] Received signed script.\n", getpid());

            char *signature;
            char *script;

            if (extract_signed_data(buffer,
                                    &signature,
                                    &script) != 0)
            {
                char response[] =
                    "STATUS : INVALID\n"
                    "Missing Base64 signature in the first line.\n";

                send_message(client_fd,
                             response,
                             strlen(response) + 1);

                close_connection(client_fd);
                exit(EXIT_FAILURE);
            }

            printf("[PID %d] Signature length : %zu bytes\n",
                   getpid(),
                   strlen(signature));

            printf("[PID %d] Script length    : %zu bytes\n", getpid(), strlen(script));

	    char response[MAX_OUTPUT_SIZE + 256];
	    char matched_certificate[256] = "";

            VerifyStatus status;

	    status = verify_signed_script("certs", signature, script, matched_certificate, sizeof(matched_certificate));

if (status == VERIFY_SUCCESS)
{
    char output[MAX_OUTPUT_SIZE];

    if (execute_script(script, output, sizeof(output)) == 0) {
	    snprintf(response, sizeof(response),
             "[PID %d]\n"
             "STATUS : VALID\n"
             "Verified using: %s\n\n"
             "Script Output:\n%.*s",
             getpid(),
             matched_certificate,
             (int)strlen(output),
             output);
    } else {
        snprintf(response,
                 sizeof(response),
                 "[PID %d]\n"
                 "STATUS : VALID\n\n"
                 "Script execution failed.\n",
                 getpid());
    }
}
else
{
    switch (status)
    {
        case VERIFY_INVALID_BASE64:

            snprintf(response,
                     sizeof(response),
                     "[PID %d]\n"
                     "STATUS : INVALID\n"
                     "Invalid Base64 signature.\n",
                     getpid());

            break;

        case VERIFY_NO_CODESIGN_EKU:

            snprintf(response,
                     sizeof(response),
                     "[PID %d]\n"
                     "STATUS : INVALID\n"
                     "Certificate does not contain Code Signing EKU.\n",
                     getpid());

            break;

        case VERIFY_CERT_LOAD_FAILED:

            snprintf(response,
                     sizeof(response),
                     "[PID %d]\n"
                     "STATUS : INVALID\n"
                     "Failed to load trusted certificate.\n",
                     getpid());

            break;

        case VERIFY_SIGNATURE_FAILED:
        default:

            snprintf(response,
                     sizeof(response),
                     "[PID %d]\n"
                     "STATUS : INVALID\n"
                     "Signature verification failed.\n",
                     getpid());

            break;
    }
}

            send_message(client_fd, response, strlen(response) + 1);

            close_connection(client_fd);

            exit(EXIT_SUCCESS);
        }

        /* Parent process */

        close_connection(client_fd);
    }

    close_connection(server_fd);

    return EXIT_SUCCESS;
}
