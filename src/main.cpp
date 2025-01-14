#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <string>
#include <map>
#include <conio.h>
#include <algorithm>
#include <iomanip>
#include <hidusage.h>

// Add this line to hide console
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

using namespace std;

// Global Variables
HINSTANCE hInst;
bool isAutoClickerRunning = false;
bool isAutoClickerPaused = false;
bool isSettingKey = false;
int basePullDuration = 1950;
int baseStopDuration = 850;
int margin = 10;
int stopGoKey = 0;
int catchKey = 0;
bool isCatchHolding = false;
bool isCatchMode = false;
HWND hwndStartStop, hwndPullDuration, hwndStopDuration, hwndStopGoKey, hwndCatchKey, hwndStatus; // Add status label
RAWINPUTDEVICE rid[1];
bool rawInputRegistered = false;
bool isProgramStarted = false;
bool isShortcutsEnabled = false;

char pullDurationBuf[16] = {0};
char stopDurationBuf[16] = {0};

// Forward Declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void autoClickerThread();
void autoClickerPausedThread();
string getKeyName(int vkCode);
void autoClicker(int basePullDuration, int baseStopDuration, int margin);
void autoClickerPaused();
bool readSettingsFromRegistry();
void writeSettingsToRegistry();
void RegisterKeyboardInput(HWND hwnd);

string getKeyName(int vkCode)
{
    if (vkCode == 0)
    {
        return "Unknown";
    }

    static map<int, string> keyMap = {
        {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"}, {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"}, {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}, {VK_F13, "F13"}, {VK_F14, "F14"}, {VK_F15, "F15"}, {VK_F16, "F16"}, {VK_UP, "Up Arrow"}, {VK_DOWN, "Down Arrow"}, {VK_LEFT, "Left Arrow"}, {VK_RIGHT, "Right Arrow"}, {VK_PRIOR, "Page Up"}, {VK_NEXT, "Page Down"}, {VK_HOME, "Home"}, {VK_END, "End"}, {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"}, {VK_ESCAPE, "Escape"}, {VK_TAB, "Tab"}, {VK_CAPITAL, "Caps Lock"}, {VK_SHIFT, "Shift"}, {VK_CONTROL, "Control"}, {VK_MENU, "Alt"}, {VK_SPACE, "Space"}, {VK_RETURN, "Enter"}, {VK_BACK, "Backspace"}, {VK_NUMLOCK, "Num Lock"}, {VK_SCROLL, "Scroll Lock"}, {VK_OEM_1, ";"}, {VK_OEM_PLUS, "="}, {VK_OEM_COMMA, ","}, {VK_OEM_MINUS, "-"}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"}, {VK_OEM_3, "`"}, {VK_OEM_4, "["}, {VK_OEM_5, "\\"}, {VK_OEM_6, "]"}, {VK_OEM_7, "'"}, {VK_OEM_8, "OEM_8"}, {VK_NUMPAD0, "Numpad 0"}, {VK_NUMPAD1, "Numpad 1"}, {VK_NUMPAD2, "Numpad 2"}, {VK_NUMPAD3, "Numpad 3"}, {VK_NUMPAD4, "Numpad 4"}, {VK_NUMPAD5, "Numpad 5"}, {VK_NUMPAD6, "Numpad 6"}, {VK_NUMPAD7, "Numpad 7"}, {VK_NUMPAD8, "Numpad 8"}, {VK_NUMPAD9, "Numpad 9"}, {VK_MULTIPLY, "Numpad *"}, {VK_ADD, "Numpad +"}, {VK_SEPARATOR, "Numpad ."}, {VK_SUBTRACT, "Numpad -"}, {VK_DECIMAL, "Numpad ."}, {VK_DIVIDE, "Numpad /"}, {VK_VOLUME_MUTE, "Volume Mute"}, {VK_VOLUME_DOWN, "Volume Down"}, {VK_VOLUME_UP, "Volume Up"}, {VK_MEDIA_NEXT_TRACK, "Next Track"}, {VK_MEDIA_PREV_TRACK, "Previous Track"}, {VK_MEDIA_STOP, "Stop Media"}, {VK_MEDIA_PLAY_PAUSE, "Play/Pause"}, {VK_BROWSER_BACK, "Browser Back"}, {VK_BROWSER_FORWARD, "Browser Forward"}, {VK_BROWSER_REFRESH, "Browser Refresh"}, {VK_BROWSER_STOP, "Browser Stop"}, {VK_BROWSER_SEARCH, "Browser Search"}, {VK_BROWSER_FAVORITES, "Browser Favorites"}, {VK_BROWSER_HOME, "Browser Home"}, {VK_SLEEP, "Sleep"}, {VK_APPS, "Menu"}, {VK_SNAPSHOT, "Print Screen"}, {VK_PAUSE, "Pause"}};

    if (keyMap.find(vkCode) != keyMap.end())
    {
        return keyMap[vkCode];
    }

    if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9'))
    {
        string key = "";
        key += static_cast<char>(vkCode);
        return key;
    }

    char keyName[256];
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

    bool isExtended = false;
    switch (vkCode)
    {
    case VK_RIGHT:
    case VK_LEFT:
    case VK_UP:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
        isExtended = true;
        break;
    default:
        isExtended = false;
    }

    LONG lparam = (scanCode << 16);
    if (isExtended)
    {
        lparam |= 0x01000000;
    }

    if (GetKeyNameTextA(lparam, keyName, sizeof(keyName)) > 0)
    {
        string name(keyName);
        if (!name.empty())
        {
            name[0] = toupper(name[0]);
        }
        return name;
    }

    return "Unknown";
}

bool readSettingsFromRegistry()
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\MyClicker"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dataSize = sizeof(DWORD);
        DWORD val1, val2, val3, val4;
        RegQueryValueEx(hKey, TEXT("PullDuration"), NULL, NULL, (LPBYTE)&val1, &dataSize);
        RegQueryValueEx(hKey, TEXT("StopDuration"), NULL, NULL, (LPBYTE)&val2, &dataSize);
        RegQueryValueEx(hKey, TEXT("StopGoKey"), NULL, NULL, (LPBYTE)&val3, &dataSize);
        RegQueryValueEx(hKey, TEXT("CatchKey"), NULL, NULL, (LPBYTE)&val4, &dataSize);
        basePullDuration = val1;
        baseStopDuration = val2;
        stopGoKey = val3;
        catchKey = val4;
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

void writeSettingsToRegistry()
{
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\MyClicker"), 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp) == ERROR_SUCCESS)
    {
        DWORD val1 = basePullDuration;
        DWORD val2 = baseStopDuration;
        DWORD val3 = stopGoKey;
        DWORD val4 = catchKey;
        RegSetValueEx(hKey, TEXT("PullDuration"), 0, REG_DWORD, (BYTE *)&val1, sizeof(val1));
        RegSetValueEx(hKey, TEXT("StopDuration"), 0, REG_DWORD, (BYTE *)&val2, sizeof(val2));
        RegSetValueEx(hKey, TEXT("StopGoKey"), 0, REG_DWORD, (BYTE *)&val3, sizeof(val3));
        RegSetValueEx(hKey, TEXT("CatchKey"), 0, REG_DWORD, (BYTE *)&val4, sizeof(val4));
        RegCloseKey(hKey);
    }
}

void RegisterKeyboardInput(HWND hwnd)
{
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwnd;

    if (RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        rawInputRegistered = true;
    }
    else
    {
        MessageBoxA(NULL, "Failed to register raw input device", "Error", MB_ICONERROR);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    srand(time(nullptr));

    bool settingsLoaded = readSettingsFromRegistry();

    const char CLASS_NAME[] = "AutoClickerWindowClass";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Auto Clicker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL)
    {
        MessageBoxA(NULL, "Window creation failed!", "Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void autoClicker(int basePullDuration, int baseStopDuration, int margin)
{
    while (isAutoClickerRunning)
    {
        // Hold left mouse button for pullDuration
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        
        // Check if mode changed during hold
        if (!isAutoClickerRunning) {
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));
            break;
        }
        
        this_thread::sleep_for(chrono::milliseconds(basePullDuration));
        
        // Release left mouse button
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
        
        // Check if mode changed during stop
        if (!isAutoClickerRunning) break;
        
        this_thread::sleep_for(chrono::milliseconds(baseStopDuration));
    }
}

void autoClickerPaused()
{
    while (isAutoClickerPaused)
    {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        this_thread::sleep_for(chrono::milliseconds(50));

        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    // Add these new cases before the existing cases
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE: // Add this to handle mouse movement while RMB is used
        if (isCatchHolding) {
            // Set a small delay to ensure the message is processed
            Sleep(1);
            // Re-assert left mouse button press
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));
            return 0; // Handle the message
        }
        break;

    case WM_CREATE:
    {
        hwndStartStop = CreateWindowA(
            "BUTTON", "Start",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 20, 100, 30,
            hwnd, (HMENU)1,
            hInst, NULL);

        CreateWindowA("STATIC", "Pull Duration (ms):",
                      WS_VISIBLE | WS_CHILD,
                      20, 70, 150, 20,
                      hwnd, NULL, NULL, NULL);

        snprintf(pullDurationBuf, sizeof(pullDurationBuf), "%d", basePullDuration);
        snprintf(stopDurationBuf, sizeof(stopDurationBuf), "%d", baseStopDuration);

        hwndPullDuration = CreateWindowA("EDIT",
                                         "",
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
                                         180, 70, 100, 20,
                                         hwnd, (HMENU)2, hInst, NULL);

        SetWindowTextA(hwndPullDuration, pullDurationBuf);

        CreateWindowA("STATIC", "Stop Duration (ms):",
                      WS_VISIBLE | WS_CHILD,
                      20, 110, 150, 20,
                      hwnd, NULL, NULL, NULL);

        hwndStopDuration = CreateWindowA("EDIT",
                                         "",
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
                                         180, 110, 100, 20,
                                         hwnd, (HMENU)3, hInst, NULL);

        SetWindowTextA(hwndStopDuration, stopDurationBuf);

        CreateWindowA("STATIC", "Stop and Go Key:",  // Changed from "Stop & Go Key:"
                      WS_VISIBLE | WS_CHILD,
                      20, 150, 150, 20,
                      hwnd, NULL, NULL, NULL);

        hwndStopGoKey = CreateWindowA(
            "BUTTON",
            (stopGoKey != 0) ? getKeyName(stopGoKey).c_str() : "Unknown",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            180, 150, 100, 25,
            hwnd, (HMENU)4, hInst, NULL);

        CreateWindowA("STATIC", "Catch Key:",
                      WS_VISIBLE | WS_CHILD,
                      20, 190, 150, 20,
                      hwnd, NULL, NULL, NULL);

        hwndCatchKey = CreateWindowA(
            "BUTTON",
            (catchKey != 0) ? getKeyName(catchKey).c_str() : "Unknown",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            180, 190, 100, 25,
            hwnd, (HMENU)5, hInst, NULL);

        // Move Save Settings button below Catch key
        CreateWindowA(
            "BUTTON", "Save",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            180, 230, 100, 25,  // New position: x=180, y=230
            hwnd, (HMENU)6,
            hInst, NULL);

        hwndStatus = CreateWindowA("STATIC", "Inactive",
                                 WS_VISIBLE | WS_CHILD,
                                 180, 20, 100, 30,
                                 hwnd, NULL, NULL, NULL);

        SetTimer(hwnd, 1, 50, NULL);
        RegisterKeyboardInput(hwnd);
    }
    break;

    case WM_INPUT:
    {
        if (isSettingKey || !isShortcutsEnabled)
            break;

        // If catch is holding, maintain left mouse button pressed after right click
        if (isCatchHolding) {
            // Re-press left mouse button with a small delay to ensure it stays held
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));
        }

        UINT dwSize = sizeof(RAWINPUT);
        BYTE lpb[sizeof(RAWINPUT)];

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

        RAWINPUT *raw = (RAWINPUT *)lpb;

        if (stopGoKey == 0 || catchKey == 0)
            break;

        if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            USHORT vKey = raw->data.keyboard.VKey;
            USHORT flags = raw->data.keyboard.Flags;

            if (flags == RI_KEY_MAKE)
            {
                INPUT input = {0};
                input.type = INPUT_MOUSE;

                // Handle Stop & Go key
                if (vKey == stopGoKey)
                {
                    // First, clean up any existing catch state
                    if (isCatchHolding) {
                        isCatchHolding = false;
                        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                        SendInput(1, &input, sizeof(INPUT));
                    }

                    isAutoClickerRunning = !isAutoClickerRunning;
                    SetWindowTextA(hwndStatus, isAutoClickerRunning ? "Stop & Go Active" : "Inactive");
                    if (isAutoClickerRunning)
                    {
                        thread(autoClickerThread).detach();
                    }
                    else 
                    {
                        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                        SendInput(1, &input, sizeof(INPUT));
                    }
                }
                // Handle Catch key
                else if (vKey == catchKey)
                {
                    // First, clean up existing Stop & Go state if needed
                    if (isAutoClickerRunning)
                    {
                        isAutoClickerRunning = false;
                        this_thread::sleep_for(chrono::milliseconds(50));
                    }

                    // Toggle catch state
                    isCatchHolding = !isCatchHolding;
                    
                    // Apply the new state
                    input.mi.dwFlags = isCatchHolding ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(INPUT));
                    SetWindowTextA(hwndStatus, isCatchHolding ? "Catch Active" : "Inactive");
                }
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case 1: // Start/Stop button
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (!isShortcutsEnabled)
                {
                    char pullBuf[16] = {0}, stopBuf[16] = {0};
                    GetWindowTextA(hwndPullDuration, pullBuf, sizeof(pullBuf));
                    GetWindowTextA(hwndStopDuration, stopBuf, sizeof(stopBuf));

                    basePullDuration = atoi(pullBuf);
                    baseStopDuration = atoi(stopBuf);

                    isShortcutsEnabled = true;
                    SetWindowTextA(hwndStartStop, "Stop");
                }
                else
                {
                    // Clean up all states when stopping
                    isShortcutsEnabled = false;
                    isAutoClickerRunning = false;
                    isCatchHolding = false;
                    SetWindowTextA(hwndStartStop, "Start");
                    SetWindowTextA(hwndStatus, "Inactive");
                    
                    // Release mouse button if it was held
                    INPUT input = {0};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(INPUT));
                }
            }
            break;

        // Add new save button handler
        case 6: // Save Settings button
            if (HIWORD(wParam) == BN_CLICKED)
            {
                char pullBuf[16] = {0}, stopBuf[16] = {0};
                GetWindowTextA(hwndPullDuration, pullBuf, sizeof(pullBuf));
                GetWindowTextA(hwndStopDuration, stopBuf, sizeof(stopBuf));

                basePullDuration = atoi(pullBuf);
                baseStopDuration = atoi(stopBuf);

                pullDurationBuf[sizeof(pullDurationBuf) - 1] = '\0';
                stopDurationBuf[sizeof(stopDurationBuf) - 1] = '\0';

                strncpy_s(pullDurationBuf, sizeof(pullDurationBuf), pullBuf, _TRUNCATE);
                strncpy_s(stopDurationBuf, sizeof(stopDurationBuf), stopBuf, _TRUNCATE);

                writeSettingsToRegistry();
                MessageBoxA(hwnd, "Settings saved successfully!", "Success", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case 4:
        {
            isSettingKey = true;
            MessageBoxA(hwnd, "Press any key for Stop & Go", "Set Key", MB_OK | MB_ICONINFORMATION);

            bool keySet = false;
            while (!keySet)
            {
                Sleep(10);

                for (int vk = 1; vk < 256; vk++)
                {
                    if (GetAsyncKeyState(vk) & 0x8000)
                    {
                        if (vk == catchKey)
                        {
                            MessageBoxA(hwnd, "This key is already used for Catch!", "Warning", MB_OK | MB_ICONWARNING);
                            continue;
                        }

                        stopGoKey = vk;
                        string keyName = getKeyName(stopGoKey);

                        if (keyName != "Unknown")
                        {
                            keySet = true;
                            SetWindowTextA(hwndStopGoKey, keyName.c_str());
                            string msg = "Stop & Go key set to: " + keyName;
                            MessageBoxA(hwnd, msg.c_str(), "Key Set", MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                    }
                }
            }

            isSettingKey = false;
            writeSettingsToRegistry();
        }
        break;

        case 5:
        {
            isSettingKey = true;
            MessageBoxA(hwnd, "Press any key for Catch", "Set Key", MB_OK | MB_ICONINFORMATION);

            bool keySet = false;
            while (!keySet)
            {
                Sleep(10);

                for (int vk = 1; vk < 256; vk++)
                {
                    if (GetAsyncKeyState(vk) & 0x8000)
                    {
                        if (vk == stopGoKey)
                        {
                            MessageBoxA(hwnd, "This key is already used for Stop & Go!", "Warning", MB_OK | MB_ICONWARNING);
                            continue;
                        }

                        catchKey = vk;
                        string keyName = getKeyName(catchKey);

                        if (keyName != "Unknown")
                        {
                            keySet = true;
                            SetWindowTextA(hwndCatchKey, keyName.c_str());
                            string msg = "Catch key set to: " + keyName;
                            MessageBoxA(hwnd, msg.c_str(), "Key Set", MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                    }
                }
            }

            isSettingKey = false;
            writeSettingsToRegistry();
        }
        break;
        }
    }
    break;

    case WM_TIMER:
    {
        if (isSettingKey)
        {
            break;
        }
    }
    break;

    case WM_DESTROY:
        writeSettingsToRegistry();
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void autoClickerThread()
{
    autoClicker(basePullDuration, baseStopDuration, margin);
}

void autoClickerPausedThread()
{
    autoClickerPaused();
}
