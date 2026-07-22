#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <stddef.h>


#ifndef CRYPTO_H
#define CRYPTO_H

int extract_signed_data(char *buffer, char **signature, char **script);

X509 *load_x509_certificate(const char *cert_path);

EVP_PKEY *extract_public_key(X509 *cert);

unsigned char *decode_signature(const char *signature_b64, size_t *signature_len);

int verify_signature(EVP_PKEY *public_key, const unsigned char *signature, size_t signature_len, const unsigned char *data, size_t data_len);

int verify_signed_script(const char *certificate_directory, const char *signature_b64, const char *script);

#endif
