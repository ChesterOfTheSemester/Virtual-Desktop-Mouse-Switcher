#undef UNICODE
#include <windows.h>

HHOOK hMouseHook;
HANDLE hMutex;

// Key combo
void SimulateKeyCombo(BYTE key) {
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(key, 0, 0, 0);
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}

// Mouse event
LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    static bool isKeyComboActive = false;

    if (nCode == HC_ACTION && wParam == WM_XBUTTONDOWN) {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;
        if (isKeyComboActive && ((p->mouseData >> 16) == XBUTTON1 || (p->mouseData >> 16) == XBUTTON2)) return 1;
        isKeyComboActive = true;
        SimulateKeyCombo((p->mouseData >> 16) == XBUTTON1 ? VK_RIGHT : VK_LEFT);
        isKeyComboActive = false;
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// Hide window
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_USER + 1: // Tray icon message
            if (lParam == WM_RBUTTONDOWN) {
                // Create and display a context menu
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1, "Exit");
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
            break;
        case WM_COMMAND: // Handle menu item selection
            switch (LOWORD(wParam)) { case 1: DestroyWindow(hwnd); break; }
            break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Ensure single instance
    hMutex = CreateMutex(NULL, TRUE, "Global\\ClippyVD");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    // Register hidden window class
    const char g_szClassName[] = "ClippyVD";
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szClassName;
    if (!RegisterClassEx(&wc)) return 0;

    // Tray
    HWND hwnd = CreateWindowEx(0, g_szClassName, "ClippyVD", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, "Clippy VD | Virtual Desktop Macro");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Mouse
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseEvent, NULL, 0);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Deallocate
    Shell_NotifyIcon(NIM_DELETE, &nid);
    UnhookWindowsHookEx(hMouseHook);
    CloseHandle(hMutex);
    return msg.wParam;
}
