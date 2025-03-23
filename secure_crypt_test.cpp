#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <random>
#include <sstream>
#include <functional>

int fud = 99999;

// Global hook handle
HHOOK keyboardHook;
std::string inputBuffer;
std::string secretKey;
bool running = true;
HANDLE hConsole;
bool keyCaptured = false;
bool paddingAdded = false;

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

// Function declarations
void SetupConsole();
void ProcessKeyboard();
void CleanupResources();
std::string XOREncryptDecrypt(char input, std::string key, size_t pos);
std::string GeneratePassword(int length, const std::string& seed);
std::string GeneratePadding(int length);

int main() {
    SetupConsole();
    ProcessKeyboard();
    CleanupResources();
    return 0;
}

unsigned int StringToDecimal(const std::string& input) {
    unsigned long seedValue = std::stoul(input); // Use stoul for unsigned conversion
    return static_cast<unsigned int>(seedValue);
}

std::string GeneratePassword(int length, const std::string& seed) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+";
    std::string password;

    // Convert the string seed to a numeric value using a hash function
    std::hash<std::string> hasher;
    unsigned int numericSeed = static_cast<unsigned int>(hasher(seed));

    // Seed the random number generator with the numeric seed
    std::mt19937 rng(numericSeed);
    std::uniform_int_distribution<int> dist(0, characters.size() - 1);

    for (int i = 0; i < length; ++i) {
        password += characters[dist(rng)];
    }
    return password;
}

std::string GeneratePadding(int length) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string padding;

    // Seed the random number generator with a random device
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, characters.size() - 1);

    for (int i = 0; i < length; ++i) {
        padding += characters[dist(rng)];
    }
    return padding;
}

std::string XOREncryptDecrypt(const std::string& input, const std::string& key) {
    if (key.empty()) {
        std::cerr << "Encryption key cannot be empty!" << std::endl;
        return "";
    }

    std::string output;
    for (size_t i = 0; i < input.size(); ++i) {
        char encryptedChar = input[i];
        for (char k : key) {
            encryptedChar ^= k;
        }
        output += encryptedChar;
    }
    return output;
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
    std::cout << "  USB KEYBOARD SECURE MESSAGE CONSOLE ACTIVATED  " << std::endl;
    std::cout << "=================================================" << std::endl;
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


        if (key == VK_RETURN) { // Enter
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
                if (!keyCaptured) {
                    secretKey += buffer[0];
                    std::cout << '*'; // Display '*' instead of the actual character
                    if (secretKey.size() == 8) {
                        keyCaptured = true;
                        std::cout << "\nSecret key captured. You can now type your message.\n";
                        secretKey = GeneratePassword(fud, secretKey);
                    }
                } else {
                    if (!paddingAdded) {
                        std::string padding = GeneratePadding(fud); // Generate padding
                        inputBuffer += padding;
                        paddingAdded = true;
                    }
                    std::cout << XOREncryptDecrypt(buffer[0], secretKey, inputBuffer.size());
                    inputBuffer += buffer[0];
                }
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
    std::cout << "Please type the 8-character secret key:\n";

    // Message loop to keep the program running and process Windows messages
    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Exit message
    std::cout << "\n\nExiting..." << std::endl;
    Sleep(1000);
}

void CleanupResources() {
    // Unhook the keyboard hook
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

std::string XOREncryptDecrypt(char input, std::string key, size_t pos) {
    char encryptedChar = input ^ key[pos % key.length()];
    std::ostringstream oss;
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)encryptedChar;
    return oss.str();
}
