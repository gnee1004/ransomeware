// walker.h - 디렉토리 재귀 순회 함수 헤더
// EVP 방식 AES 암호화를 위해 key, iv는 unsigned char* 사용

#ifndef WALKER_H
#define WALKER_H

void scan_directory(const char* dir, const unsigned char* key, const unsigned char* iv);

#endif // WALKER_H
