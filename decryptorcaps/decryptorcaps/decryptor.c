#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "decryptor.h"

#define FILENAME_PADDING 512

static const unsigned char xor_mask = 0xA5;

static const unsigned char obfuscated_key[32] = {
    0x14, 0x5d, 0x52, 0x3b, 0xd8, 0xa7, 0xa4, 0xeb,
    0x9d, 0x00, 0x6c, 0x42, 0x26, 0x8d, 0xd3, 0x18,
    0x5f, 0x12, 0x22, 0xad, 0xb8, 0xc2, 0x1a, 0x5f,
    0xac, 0x71, 0x96, 0x9e, 0xce, 0xc7, 0x6b, 0xd9
};

static const unsigned char obfuscated_iv[16] = {
    0x99, 0xdb, 0xbc, 0xef, 0xa5, 0x77, 0xf0, 0x16,
    0x33, 0x84, 0x5b, 0xcb, 0x01, 0xac, 0x75, 0x69
};

void restore_key(unsigned char* key_out) {
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        key_out[i] = obfuscated_key[i] ^ xor_mask;
    }
}

void restore_iv(unsigned char* iv_out) {
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        iv_out[i] = obfuscated_iv[i] ^ xor_mask;
    }
}

int decrypt_file(const char* encrypted_path, const unsigned char* key, const unsigned char* iv) {
    FILE* in = fopen(encrypted_path, "rb");
    if (!in) return 0;

    fseek(in, 0, SEEK_END);
    long filesize = ftell(in);
    rewind(in);

    if (filesize <= FILENAME_PADDING) {
        fclose(in);
        return 0;
    }

    unsigned char* encrypted_data = malloc(filesize);
    if (!encrypted_data) {
        fclose(in);
        return 0;
    }
    fread(encrypted_data, 1, filesize, in);
    fclose(in);

    unsigned char* decrypted_data = malloc(filesize);
    if (!decrypted_data) {
        free(encrypted_data);
        return 0;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(encrypted_data); free(decrypted_data);
        return 0;
    }

    int len = 0, decrypted_len = 0;
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) ||
        !EVP_DecryptUpdate(ctx, decrypted_data, &len, encrypted_data, (int)filesize)) {
        EVP_CIPHER_CTX_free(ctx);
        free(encrypted_data); free(decrypted_data);
        return 0;
    }
    decrypted_len = len;
    if (!EVP_DecryptFinal_ex(ctx, decrypted_data + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        free(encrypted_data); free(decrypted_data);
        return 0;
    }
    decrypted_len += len;

    EVP_CIPHER_CTX_free(ctx);
    free(encrypted_data);

    char original_path[FILENAME_PADDING] = { 0 };
    memcpy(original_path, decrypted_data + decrypted_len - FILENAME_PADDING, FILENAME_PADDING - 1);
    decrypted_len -= FILENAME_PADDING;

    FILE* out = fopen(original_path, "wb");
    if (!out) {
        free(decrypted_data);
        return 0;
    }
    fwrite(decrypted_data, 1, decrypted_len, out);
    fclose(out);
    free(decrypted_data);

    DeleteFileA(encrypted_path);
    return 1;
}
