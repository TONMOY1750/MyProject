#include "crypto.h"

/* XOR Cipher for stream encryption and decryption */
void xor_cipher(const unsigned char *input, unsigned char *output, size_t len, unsigned char key) {
    for (size_t i = 0; i < len; i++) {
        output[i] = input[i] ^ key;
    }
}
