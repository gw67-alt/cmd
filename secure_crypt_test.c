#include <windows.h>
#include <iostream>
#include <string>

// Global hook handle
HHOOK keyboardHook;
std::string inputBuffer;
bool running = true;
HANDLE hConsole;

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

// Function declarations
void SetupConsole();
void ProcessKeyboard();
void CleanupResources();
char XOREncryptDecrypt(char input, char key);

int main() {
    SetupConsole();
    ProcessKeyboard();
    CleanupResources();
    return 0;
}

void SetupConsole() {
    // Set console title
    SetConsoleTitle("USB Keyboard Console Bridge");

    // Maximize console window
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_MAXIMIZE);

    // Clear console and display welcome message
    system("cls");
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "=================================================" << std::endl;
    std::cout << "      USB KEYBOARD CONSOLE BRIDGE ACTIVATED      " << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "Press ESC key to exit the application\n" << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // If nCode is less than zero, we must pass the message to the next hook
    if (nCode < 0) {
        return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
    }

    // Only process keydown messages
    if (wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        int key = kbdStruct->vkCode;

        // Check for ESC key to exit
        if (key == VK_ESCAPE) {
            running = false;
        }
        // Handle special keys
        else if (key == VK_BACK) { // Backspace
            if (!inputBuffer.empty()) {
                inputBuffer.pop_back();
                // Update display (clear line and rewrite)
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(hConsole, &csbi);
                COORD cursorPos = { 0, csbi.dwCursorPosition.Y };
                SetConsoleCursorPosition(hConsole, cursorPos);
                // Clear the line
                std::string spaces(csbi.dwSize.X, ' ');
                std::cout << spaces;
                SetConsoleCursorPosition(hConsole, cursorPos);
                for (char c : inputBuffer) {
                    std::cout << XOREncryptDecrypt(c, 0xAA);
                }
            }
        }
        else if (key == VK_RETURN) { // Enter
            std::cout << std::endl;
            inputBuffer.clear();
        }
        else {
            // Convert virtual key to character
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);
            char buffer[2] = {0};
            WORD character = 0;
            // Convert key to ASCII character
            int result = ToAscii(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, &character, 0);
            if (result == 1) {
                buffer[0] = (char)character;
                std::cout << XOREncryptDecrypt(buffer[0], 0xAA);
                inputBuffer += buffer[0];
            }
        }

        // Consume the keystroke (prevent Windows from processing it)
        return 1;
    }

    // Pass the hook to the next hook in the chain
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void ProcessKeyboard() {
    // Set up low-level keyboard hook
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    if (!keyboardHook) {
        std::cerr << "Failed to set keyboard hook! Error: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Keyboard hook installed. All keystrokes are now redirected." << std::endl;

    // Message loop to keep the program running and process Windows messages
    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Exit message
    std::cout << "\n\nExiting USB Keyboard Console Bridge..." << std::endl;
    Sleep(1000);
}

void CleanupResources() {
    // Unhook the keyboard hook
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

char XOREncryptDecrypt(char input, char key) {
    return input ^ key;
}
