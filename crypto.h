#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

#define FIXED_SESSION_KEY 0x5A

/* Simple XOR Stream Cipher Encryption/Decryption */
void xor_cipher(const unsigned char *input, unsigned char *output, size_t len, unsigned char key);

#endif // CRYPTO_H
