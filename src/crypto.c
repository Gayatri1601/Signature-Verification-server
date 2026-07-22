#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <openssl/evp.h>
#include "crypto.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int extract_signed_data(char *buffer, char **signature, char **script)
{
    printf("[INFO] Extracting signature and script from received data...\n");

    if (buffer == NULL) {
        printf("[ERROR] Input buffer is NULL.\n");
        return -1;
    }

    char *newline = strchr(buffer, '\n');

    if (newline == NULL) {
        printf("[ERROR] Invalid signed script format.\n");
        return -1;
    }

    *newline = '\0';

    *signature = buffer;
    *script = newline + 1;

    printf("[INFO] Signature extracted successfully.\n");
    printf("[INFO] Script extracted successfully.\n");

    return 0;
}

X509 *load_x509_certificate(const char *cert_path)
{
    printf("[INFO] Loading X.509 certificate...\n");

    FILE *fp = fopen(cert_path, "r");

    if (fp == NULL) {
        perror("[ERROR] Failed to open certificate");
        return NULL;
    }

    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);

    fclose(fp);

    if (cert == NULL) {
        printf("[ERROR] Failed to parse X.509 certificate.\n");
        return NULL;
    }

    printf("[INFO] Certificate loaded successfully.\n");

    return cert;
}

EVP_PKEY *extract_public_key(X509 *cert)
{
    printf("[INFO] Extracting public key from certificate...\n");

    if (cert == NULL) {
        printf("[ERROR] Certificate is NULL.\n");
        return NULL;
    }

    EVP_PKEY *public_key = X509_get_pubkey(cert);

    if (public_key == NULL) {
        printf("[ERROR] Failed to extract public key.\n");
        return NULL;
    }

    printf("[INFO] Public key extracted successfully.\n");

    return public_key;
}

unsigned char *decode_signature(const char *signature_b64, size_t *signature_len)
{
    printf("[INFO] Decoding Base64 signature...\n");

    if (signature_b64 == NULL || signature_len == NULL) {
        printf("[ERROR] Invalid input to Base64 decoder.\n");
        return NULL;
    }

    size_t encoded_len = strlen(signature_b64);

    size_t max_decoded_len = (encoded_len * 3) / 4;

    unsigned char *decoded = malloc(max_decoded_len);

    if (decoded == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        return NULL;
    }

    int decoded_len = EVP_DecodeBlock(
        decoded,
        (const unsigned char *)signature_b64,
        encoded_len);

    if (decoded_len < 0) {
        printf("[ERROR] Failed to decode Base64 signature.\n");
        free(decoded);
        return NULL;
    }

    /* Remove Base64 padding bytes */
    if (encoded_len > 0 && signature_b64[encoded_len - 1] == '=') {
        decoded_len--;

        if (encoded_len > 1 &&
            signature_b64[encoded_len - 2] == '=') {
            decoded_len--;
        }
    }

    *signature_len = decoded_len;

    printf("[INFO] Signature decoded successfully.\n");

    return decoded;
}

int verify_signature(EVP_PKEY *public_key,
                     const unsigned char *signature,
                     size_t signature_len,
                     const unsigned char *data,
                     size_t data_len)
{
    printf("[INFO] Verifying digital signature...\n");

    if (public_key == NULL ||
        signature == NULL ||
        data == NULL) {
        printf("[ERROR] Invalid input to signature verification.\n");
        return 0;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (ctx == NULL) {
        printf("[ERROR] Failed to create digest context.\n");
        return 0;
    }

    printf("[INFO] Initializing SHA-256 verification...\n");

    if (EVP_DigestVerifyInit(ctx,
                             NULL,
                             EVP_sha256(),
                             NULL,
                             public_key) != 1) {
        printf("[ERROR] Failed to initialize signature verification.\n");
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    printf("[INFO] Computing SHA-256 hash of the script...\n");

    if (EVP_DigestVerifyUpdate(ctx,
                               data,
                               data_len) != 1) {
        printf("[ERROR] Failed while processing script data.\n");
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    printf("[INFO] Comparing computed hash with the received signature...\n");

    int result = EVP_DigestVerifyFinal(ctx,
                                       signature,
                                       signature_len);

    EVP_MD_CTX_free(ctx);

    if (result == 1) {
        printf("[INFO] Signature verification successful.\n");
    }
    else if (result == 0) {
        printf("[ERROR] Signature verification failed. The script may have been modified or signed with a different private key.\n");
    }
    else {
        printf("[ERROR] OpenSSL encountered an error during signature verification.\n");
    }

    return (result == 1);
}

int verify_signed_script(const char *certificate_directory, const char *signature_b64, const char *script)
{
    printf("\n========== Starting Signature Verification ==========\n");

    unsigned char *decoded_signature = NULL;
    size_t signature_len = 0;

    /* Decode the signature only once */
    decoded_signature = decode_signature(signature_b64, &signature_len);
    if (decoded_signature == NULL) {
        printf("[ERROR] Failed to decode Base64 signature.\n");
        return 0;
    }

    DIR *dir = opendir(certificate_directory);
    if (dir == NULL) {
        printf("[ERROR] Failed to open certificate directory: %s\n", certificate_directory);

        free(decoded_signature);
        return 0;
    }

    printf("[INFO] Searching trusted certificates...\n");

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Ignore private keys */
        if (strncmp(entry->d_name, "certificate", 11) != 0) {
            continue;
        }

        char cert_path[PATH_MAX];

        snprintf(cert_path, sizeof(cert_path), "%s/%s", certificate_directory, entry->d_name);

        printf("\n----------------------------------------\n");
        printf("[INFO] Checking certificate: %s\n", entry->d_name);
        printf("----------------------------------------\n");

        X509 *cert = load_x509_certificate(cert_path);

        if (cert == NULL) {
            printf("[WARNING] Failed to load %s. Skipping...\n",
                   entry->d_name);
            continue;
        }

        EVP_PKEY *public_key = extract_public_key(cert);

        if (public_key == NULL) {
            printf("[WARNING] Failed to extract public key from %s\n",
                   entry->d_name);

            X509_free(cert);
            continue;
        }

        int verified = verify_signature(public_key, decoded_signature, signature_len, (const unsigned char *)script, strlen(script));

        EVP_PKEY_free(public_key);
        X509_free(cert);

        if (verified) {
            printf("[SUCCESS] Signature verified using %s\n", entry->d_name);

            free(decoded_signature);
            closedir(dir);

            printf("========== Signature Verification Finished ==========\n\n");

            return 1;
        }

        printf("[INFO] Signature did not match %s\n",
               entry->d_name);
    }

    closedir(dir);
    free(decoded_signature);

    printf("\n[ERROR] No trusted certificate matched the signature.\n");
    printf("========== Signature Verification Finished ==========\n\n");

    return 0;
}
