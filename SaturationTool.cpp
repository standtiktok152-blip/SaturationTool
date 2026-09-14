// SaturationTool.cpp - минимальная версия без лишних зависимостей
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <cmath>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

float g_Saturation = 1.5f;
float g_Brightness = 1.0f;
float g_Contrast = 1.0f;
float g_Gamma = 1.0f;
bool g_Enabled = true;
HWND g_hWnd = NULL;
NOTIFYICONDATAW g_nid = {};
bool g_Running = true;

void ApplyColorCorrection() {
    if (!g_Enabled) return;
    HDC hdc = GetDC(NULL);
    if (!hdc) return;
    WORD ramp[768];
    for (int i = 0; i < 256; i++) {
        float val = (float)i / 255.0f;
        val = powf(val, 1.0f / g_Gamma);
        val *= g_Brightness;
        val = (val - 0.5f) * g_Contrast + 0.5f;
        float r = val, g = val, b = val;
        float gray = 0.299f * r + 0.587f * g + 0.114f * b;
        r = gray + (r - gray) * g_Saturation;
        g = gray + (g - gray) * g_Saturation;
        b = gray + (b - gray) * g_Saturation;
        if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
        if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
        if (b < 0.0f) b = 0.0f; if (b > 1.0f) b = 1.0f;
        ramp[i] = (WORD)(r * 65535.0f);
        ramp[i + 256] = (WORD)(g * 65535.0f);
        ramp[i + 512] = (WORD)(b * 65535.0f);
    }
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

void ResetColorCorrection() {
    HDC hdc = GetDC(NULL);
    if (!hdc) return;
    WORD ramp[768];
    for (int i = 0; i < 256; i++) {
        WORD val = (WORD)(i * 256);
        ramp[i] = val;
        ramp[i + 256] = val;
        ramp[i + 512] = val;
    }
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

void AddTrayIcon() {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 1;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpyW(g_nid.szTip, L"Saturation Tool");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void ShowBalloon(const wchar_t* title, const wchar_t* text) {
    g_nid.uFlags = NIF_INFO;
    lstrcpynW(g_nid.szInfoTitle, title, 64);
    lstrcpynW(g_nid.szInfo, text, 256);
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_USER + 1:
        if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Saturation + (Ctrl+Alt+Up)");
            AppendMenuW(hMenu, MF_STRING, 2, L"Saturation - (Ctrl+Alt+Down)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 3, L"Reset (Ctrl+Alt+R)");
            AppendMenuW(hMenu, MF_STRING, 4, g_Enabled ? L"Disable (Ctrl+Alt+E)" : L"Enable (Ctrl+Alt+E)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 5, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: if (g_Saturation < 3.0f) g_Saturation += 0.1f; ApplyColorCorrection(); break;
        case 2: if (g_Saturation > 1.0f) g_Saturation -= 0.1f; ApplyColorCorrection(); break;
        case 3: g_Saturation = 1.0f; g_Brightness = 1.0f; g_Contrast = 1.0f; g_Gamma = 1.0f; ApplyColorCorrection(); break;
        case 4: g_Enabled = !g_Enabled; if (g_Enabled) ApplyColorCorrection(); else ResetColorCorrection(); break;
        case 5: g_Running = false; PostQuitMessage(0); break;
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: if (g_Saturation < 3.0f) g_Saturation += 0.1f; ApplyColorCorrection(); break;
        case 2: if (g_Saturation > 1.0f) g_Saturation -= 0.1f; ApplyColorCorrection(); break;
        case 3: g_Saturation = 1.0f; g_Brightness = 1.0f; g_Contrast = 1.0f; g_Gamma = 1.0f; ApplyColorCorrection(); break;
        case 4: g_Enabled = !g_Enabled; if (g_Enabled) ApplyColorCorrection(); else ResetColorCorrection(); break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SaturationToolClass";
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, L"SaturationToolClass", L"Saturation Tool",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    AddTrayIcon();
    RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT, VK_UP);
    RegisterHotKey(NULL, 2, MOD_CONTROL | MOD_ALT, VK_DOWN);
    RegisterHotKey(NULL, 3, MOD_CONTROL | MOD_ALT, 'R');
    RegisterHotKey(NULL, 4, MOD_CONTROL | MOD_ALT, 'E');
    ApplyColorCorrection();
    ShowBalloon(L"Saturation Tool", L"Ctrl+Alt+Up / Down");
    MSG msg;
    while (g_Running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);
    UnregisterHotKey(NULL, 3);
    UnregisterHotKey(NULL, 4);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    ResetColorCorrection();
    return 0;
}
