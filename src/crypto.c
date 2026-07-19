#include <stdio.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include "crypto.h"

int extract_signed_data(char *buffer, char **signature, char **script)
{
    if (buffer == NULL)
    {
        return -1;
    }

    char *newline = strchr(buffer, '\n');

    if (newline == NULL)
    {
        return -1;
    }

    *newline = '\0';

    *signature = buffer;
    *script = newline + 1;

    return 0;
}
