// walker.c
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <initguid.h>
#include <knownfolders.h>
#include "decryptor.h"
#include "walker.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

int is_target_file(const char* filename) {
    return (StrStrIA(filename, ".adr") != NULL);
}

int is_system_path(const char* path) {
    return (
        StrStrIA(path, "\\Windows\\") ||
        StrStrIA(path, "\\Program Files") ||
        StrStrIA(path, "\\AppData\\")
        );
}

int scan_and_decrypt(const char* dir, const unsigned char* key, const unsigned char* iv) {
    int success = 0;
    if (is_system_path(dir)) return 0;

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char fullpath[MAX_PATH];
        snprintf(fullpath, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            success += scan_and_decrypt(fullpath, key, iv);
        }
        else {
            if (is_target_file(fd.cFileName)) {
                if (decrypt_file(fullpath, key, iv)) {
                    success++;
                }
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    return success;
}

int decrypt_all_files() {
    unsigned char key[32], iv[16];
    restore_key(key);
    restore_iv(iv);

    int total_success = 0;

    const int folders[] = {
        CSIDL_DESKTOP, CSIDL_PERSONAL, CSIDL_MYPICTURES,
        CSIDL_MYMUSIC, CSIDL_MYVIDEO, CSIDL_PROFILE
    };

    char path[MAX_PATH];
    for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
        if (SHGetFolderPathA(NULL, folders[i], NULL, 0, path) == S_OK) {
            total_success += scan_and_decrypt(path, key, iv);
        }
    }

    PWSTR wpath = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &wpath) == S_OK) {
        wcstombs(path, wpath, MAX_PATH);
        total_success += scan_and_decrypt(path, key, iv);
        CoTaskMemFree(wpath);
    }

    GetCurrentDirectoryA(MAX_PATH, path);
    total_success += scan_and_decrypt(path, key, iv);

    return total_success;
}
