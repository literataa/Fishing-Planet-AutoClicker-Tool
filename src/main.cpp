#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <tchar.h>
#include <string>

using namespace std;

bool isAutoClickerRunning = false;
bool isAutoClickerBackward = false;

int getRandomDuration(int base, int margin) {
    return base + (rand() % (2 * margin + 1)) - margin;
}

void clickLeftMouse(int duration) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    this_thread::sleep_for(chrono::milliseconds(duration));
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void clickRightMouse(int downDuration, int upDuration) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    this_thread::sleep_for(chrono::milliseconds(downDuration));
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
    this_thread::sleep_for(chrono::milliseconds(upDuration));
}

void autoClicker(int basePullDuration, int baseStopDuration, int margin) {
    while (isAutoClickerRunning) {
        int pullDuration = getRandomDuration(basePullDuration, margin);
        int stopDuration = getRandomDuration(baseStopDuration, margin);
        cout << "Go : " << pullDuration << ", Stop : " << stopDuration << endl << flush;
        clickLeftMouse(pullDuration);
        this_thread::sleep_for(chrono::milliseconds(stopDuration));
    }
}

void autoClickerBackward() {
    while (isAutoClickerBackward) {
        clickLeftMouse(50);
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

bool readSettingsFromRegistry(int &basePullDuration, int &baseStopDuration);
bool writeSettingsToRegistry(int basePullDuration, int baseStopDuration);

void showMenu(int &basePullDuration, int &baseStopDuration) {
    bool prevRunning = isAutoClickerRunning;
    bool prevBackward = isAutoClickerBackward;
    isAutoClickerRunning = false;
    isAutoClickerBackward = false;

    cout << "Menu:\n";
    cout << "1. Change base pull duration (current: " << basePullDuration << " ms)\n";
    cout << "2. Change base stop duration (current: " << baseStopDuration << " ms)\n";
    cout << "3. Exit menu\n";
    cout << "Enter your choice: " << flush;
    int choice;
    cin >> choice;
    switch (choice) {
        case 1:
            cout << "Enter new base pull duration (ms): ";
            cin >> basePullDuration;
            break;
        case 2:
            cout << "Enter new base stop duration (ms): ";
            cin >> baseStopDuration;
            break;
        case 3:
            // Restore status
            if (prevRunning) isAutoClickerRunning = true;
            if (prevBackward) isAutoClickerBackward = true;
            return;
        default:
            cout << "Invalid choice. Try again.\n";
    }

    writeSettingsToRegistry(basePullDuration, baseStopDuration);

    // Restore status
    if (prevRunning) isAutoClickerRunning = true;
    if (prevBackward) isAutoClickerBackward = true;
}

bool readSettingsFromRegistry(int &basePullDuration, int &baseStopDuration) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\MyClicker"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dataSize = sizeof(DWORD);
        DWORD val1 = 0, val2 = 0;
        RegQueryValueEx(hKey, _T("BasePull"), NULL, NULL, (LPBYTE)&val1, &dataSize);
        RegQueryValueEx(hKey, _T("BaseStop"), NULL, NULL, (LPBYTE)&val2, &dataSize);
        basePullDuration = (int)val1;
        baseStopDuration = (int)val2;
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool writeSettingsToRegistry(int basePullDuration, int baseStopDuration) {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\MyClicker"), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp) == ERROR_SUCCESS) {
        DWORD val1 = (DWORD)basePullDuration;
        DWORD val2 = (DWORD)baseStopDuration;
        RegSetValueEx(hKey, _T("BasePull"), 0, REG_DWORD, (BYTE*)&val1, sizeof(val1));
        RegSetValueEx(hKey, _T("BaseStop"), 0, REG_DWORD, (BYTE*)&val2, sizeof(val2));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

int main() {
    srand(time(nullptr));

    int basePullDuration = 2000;  // Durasi go (dalam milidetik)
    int baseStopDuration = 1100;  // Durasi stop (dalam milidetik)
    int margin = 10;  // Margin (dalam milidetik)

    readSettingsFromRegistry(basePullDuration, baseStopDuration);

    while (true) {
        if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) {
            if (isAutoClickerBackward) {
                isAutoClickerBackward = false;
                cout << "Catch deactive\n";
            }
            isAutoClickerRunning = !isAutoClickerRunning;
            if (isAutoClickerRunning) {
                thread(autoClicker, basePullDuration, baseStopDuration, margin).detach();
                cout << "Stop & Go active\n";
            } else {
                cout << "Stop & Go deactive\n";
            }
            this_thread::sleep_for(chrono::milliseconds(500));
        }

        if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) {
            if (isAutoClickerRunning) {
                isAutoClickerRunning = false;
                cout << "Stop & Go deactive\n";
            }
            isAutoClickerBackward = !isAutoClickerBackward;
            if (isAutoClickerBackward) {
                thread(autoClickerBackward).detach();
                cout << "Catch active\n";
            } else {
                cout << "Catch deactive\n";
            }
            this_thread::sleep_for(chrono::milliseconds(500));
        }

        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            showMenu(basePullDuration, baseStopDuration);
            this_thread::sleep_for(chrono::milliseconds(500));
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    writeSettingsToRegistry(basePullDuration, baseStopDuration);

    return 0;
}
