// SaturationTool.cpp
// Компилировать: Visual Studio 2022, Release x64, /MT /O2 /EHsc

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>
#include <shellapi.h>
#include <gdiplus.h>
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

float g_Saturation = 1.5f;
float g_Brightness = 1.0f;
float g_Contrast = 1.0f;
float g_Gamma = 1.0f;
bool g_Enabled = true;
HWND g_hWnd = nullptr;
NOTIFYICONDATAW g_nid = {};
bool g_Running = true;

void ApplyColorCorrection() {
    if (!g_Enabled) return;
    HDC hdc = GetDC(NULL);
    if (!hdc) return;
    WORD ramp[256 * 3];
    for (int i = 0; i < 256; i++) {
        float val = i / 255.0f;
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
        ramp[i]         = (WORD)(r * 65535.0f);
        ramp[i + 256]   = (WORD)(g * 65535.0f);
        ramp[i + 512]   = (WORD)(b * 65535.0f);
    }
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

void ResetColorCorrection() {
    HDC hdc = GetDC(NULL);
    if (!hdc) return;
    WORD ramp[256 * 3];
    for (int i = 0; i < 256; i++) {
        WORD val = (WORD)(i * 256);
        ramp[i] = val; ramp[i + 256] = val; ramp[i + 512] = val;
    }
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

void SaveSettings() {
    std::ofstream f("saturation_settings.ini");
    if (f) {
        f << g_Saturation << "\n" << g_Brightness << "\n"
          << g_Contrast << "\n" << g_Gamma << "\n" << g_Enabled << "\n";
    }
}

void LoadSettings() {
    std::ifstream f("saturation_settings.ini");
    if (f) { f >> g_Saturation >> g_Brightness >> g_Contrast >> g_Gamma >> g_Enabled; }
}

void RegisterHotkeys() {
    RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT, VK_UP);
    RegisterHotKey(NULL, 2, MOD_CONTROL | MOD_ALT, VK_DOWN);
    RegisterHotKey(NULL, 3, MOD_CONTROL | MOD_ALT, 'R');
    RegisterHotKey(NULL, 4, MOD_CONTROL | MOD_ALT, 'E');
}

void UnregisterHotkeys() {
    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);
    UnregisterHotKey(NULL, 3);
    UnregisterHotKey(NULL, 4);
}

void AddTrayIcon() {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 1;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"Saturation Tool");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() { Shell_NotifyIconW(NIM_DELETE, &g_nid); }

void ShowBalloon(const wchar_t* title, const wchar_t* text) {
    g_nid.uFlags = NIF_INFO;
    wcscpy_s(g_nid.szInfoTitle, title);
    wcscpy_s(g_nid.szInfo, text);
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_USER + 1:
        if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Увеличить насыщенность (Ctrl+Alt+Up)");
            AppendMenuW(hMenu, MF_STRING, 2, L"Уменьшить насыщенность (Ctrl+Alt+Down)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 3, L"Сбросить (Ctrl+Alt+R)");
            AppendMenuW(hMenu, MF_STRING, 4, g_Enabled ? L"Выключить (Ctrl+Alt+E)" : L"Включить (Ctrl+Alt+E)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 5, L"Выход");
            POINT pt; GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: g_Saturation = min(3.0f, g_Saturation + 0.1f); ApplyColorCorrection(); break;
        case 2: g_Saturation = max(1.0f, g_Saturation - 0.1f); ApplyColorCorrection(); break;
        case 3: g_Saturation=1.0f; g_Brightness=1.0f; g_Contrast=1.0f; g_Gamma=1.0f; ApplyColorCorrection(); break;
        case 4: g_Enabled = !g_Enabled; if (g_Enabled) ApplyColorCorrection(); else ResetColorCorrection(); break;
        case 5: g_Running = false; PostQuitMessage(0); break;
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: g_Saturation = min(3.0f, g_Saturation + 0.1f); ApplyColorCorrection(); break;
        case 2: g_Saturation = max(1.0f, g_Saturation - 0.1f); ApplyColorCorrection(); break;
        case 3: g_Saturation=1.0f; g_Brightness=1.0f; g_Contrast=1.0f; g_Gamma=1.0f; ApplyColorCorrection(); break;
        case 4: g_Enabled = !g_Enabled; if (g_Enabled) ApplyColorCorrection(); else ResetColorCorrection(); break;
        }
        break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    LoadSettings();
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SaturationToolClass";
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, L"SaturationToolClass", L"Saturation Tool",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    AddTrayIcon();
    RegisterHotkeys();
    ApplyColorCorrection();
    ShowBalloon(L"Saturation Tool", L"Запущено. Ctrl+Alt+Up / Down — насыщенность.");
    MSG msg;
    while (g_Running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterHotkeys();
    RemoveTrayIcon();
    ResetColorCorrection();
    SaveSettings();
    return 0;
}
