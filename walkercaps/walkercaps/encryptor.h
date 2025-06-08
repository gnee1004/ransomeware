#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <windows.h>

#define AES_KEY_SIZE 32     // AES-256 사용
#define AES_BLOCK_SIZE 16

// 암호화 함수
void encrypt_file(const char* filepath, const unsigned char* key, const unsigned char* iv);

// 키/IV 복원 함수 (난독화된 값 XOR 해제용)
void restore_key(unsigned char* key_out);
void restore_iv(unsigned char* iv_out);


#endif // ENCRYPTOR_H
