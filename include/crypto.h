#ifndef CRYPTO_H
#define CRYPTO_H

int extract_signed_data(char *buffer, char **signature, char **script);

X509 *load_certificate(const char *cert_path);

#endif
