#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <random>
#include <sstream>
#include <fstream>
#include <functional>

int fud = 9999999;

// Global hook handle
HHOOK keyboardHook;
std::string inputBuffer;        // Actual user input buffer
std::string encryptedBuffer;    // Encrypted content buffer
std::string secretKey;
bool running = true;
HANDLE hConsole;
bool keyCaptured = false;
bool paddingAdded = false;
bool decryptMode = false;       // New flag for decrypt mode
bool waitingForDecryptKey = false; // Flag for password input in decrypt mode
bool waitingForFilename = true;  // New flag for filename input in encrypt mode
bool waitingForDecryptFilename = false; // New flag for filename input in decrypt mode
std::string tempKey = "";       // Temporary storage for key during input
std::string outputFilename = ""; // Storage for custom filename
std::string decryptFilename = ""; // Storage for custom decryption filename

// Output file
std::ofstream outputFile;       // Initialize without opening

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

// Function declarations
void SetupConsole();
void ProcessKeyboard();
void CleanupResources();
char XOREncryptDecryptChar(char input, const std::string& key, size_t pos);
std::string GeneratePassword(int length, const std::string& seed);
std::string GeneratePadding(int length);
void DumpToFile(const std::string& text);
void EnterDecryptMode();
std::string XOREncryptDecrypt(const std::string& input, const std::string& key);

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

    // Use a more reasonable length to avoid memory issues
    int actualLength = std::min(length, 10000000); // Limit to 1000 characters

    for (int i = 0; i < actualLength; ++i) {
        password += characters[dist(rng)];
    }
    return password;
}

std::string GeneratePadding(int length) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string padding;

    // Use a reasonable padding size to avoid memory issues (100 chars is enough for demonstration)
    int paddingSize = 10000000;

    // Seed the random number generator with a random device
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, characters.size() - 1);

    for (int i = 0; i < paddingSize; ++i) {
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
    std::cout << "Press ESC to toggle decrypt mode..." << std::endl;
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

        // Check for ESC key to toggle decrypt mode
        if (key == VK_ESCAPE) {
            if (!decryptMode) {
                // Save current buffer and enter decrypt mode - only if we have something to save
                if (paddingAdded && encryptedBuffer.length() > 0) {
                    DumpToFile(encryptedBuffer); // Dump buffer to file
                }
                EnterDecryptMode();
            } else if (!waitingForDecryptKey && !waitingForDecryptFilename) {
                // Exit decrypt mode and return to encrypt mode
                decryptMode = false;
                system("cls");
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                std::cout << "Returning to encrypt mode..." << std::endl;
                std::cout << "Press ESC to toggle decrypt mode..." << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                keyCaptured = false;
                secretKey = "";
                paddingAdded = false;
                encryptedBuffer = "";
                waitingForFilename = true;
                outputFilename = "";
                decryptFilename = "";

                // Close the current file if open
                if (outputFile.is_open()) {
                    outputFile.close();
                }

                std::cout << "Please enter output filename: ";
            }
        }
        // Handle filename input and confirmation for encryption
        else if (key == VK_RETURN && waitingForFilename) {
            if (outputFilename.size() > 0) {
                std::cout << std::endl;

                // Open output file with the specified name
                outputFile.open(outputFilename);
                if (!outputFile.is_open()) {
                    std::cerr << "Failed to open file: " << outputFilename << std::endl;
                    // Attempt to use a default name
                    outputFilename = "output.txt";
                    outputFile.open(outputFilename);
                    if (!outputFile.is_open()) {
                        std::cerr << "Failed to open default file. Check permissions." << std::endl;
                        return 1;
                    }
                    std::cout << "Using default filename: " << outputFilename << std::endl;
                } else {
                    std::cout << "Output will be saved to: " << outputFilename << std::endl;
                }

                waitingForFilename = false;
                std::cout << "Please type the 8-character secret key:\n";
            }
        }
        // Handle filename input and confirmation for decryption
        else if (key == VK_RETURN && waitingForDecryptFilename) {
            std::cout << std::endl;

            if (decryptFilename.empty()) {
                // If no filename was entered, use default
                decryptFilename = "output.txt";
                std::cout << "Using default filename: " << decryptFilename << std::endl;
            } else {
                std::cout << "File to decrypt: " << decryptFilename << std::endl;
            }

            // Check if file exists
            std::ifstream testFile(decryptFilename);
            if (!testFile.is_open()) {
                std::cerr << "Warning: File '" << decryptFilename << "' does not exist or cannot be opened." << std::endl;
                std::cout << "Please check the filename and try again." << std::endl;
                decryptFilename = "";
                waitingForDecryptFilename = true;
                std::cout << "Enter filename to decrypt (press Enter for default 'output.txt'): ";
                return 1;
            }
            testFile.close();

            waitingForDecryptFilename = false;
            waitingForDecryptKey = true;

            std::cout << "Enter the secret key used for encryption (press Enter when done): ";
        }
        // Handle Enter key for decrypt mode password confirmation
        else if (key == VK_RETURN && waitingForDecryptKey) {
            if (tempKey.size() > 0) {
                std::cout << std::endl;
                // Use the entered key to generate the full secret key
                secretKey = GeneratePassword(10000000, tempKey);
                waitingForDecryptKey = false;

                // Process decryption with the entered key
                // Open and read the encrypted file
                std::ifstream inputFile(decryptFilename);
                if (!inputFile.is_open()) {
                    std::cerr << "Failed to open " << decryptFilename << " for decryption!" << std::endl;
                    return 1;
                }

                std::string encryptedContent;
                std::string line;
                while (std::getline(inputFile, line)) {
                    encryptedContent += line;
                }
                inputFile.close();

                if (encryptedContent.empty()) {
                    std::cout << "No encrypted content found in " << decryptFilename << std::endl;
                    return 1;
                }

                // Skip the padding at the beginning - only if content is long enough
                int paddingSize = 10000000; // We're using a fixed padding size of 100 now
                if (encryptedContent.length() > paddingSize) {
                    encryptedContent = encryptedContent.substr(paddingSize);
                }

                // Decrypt the content - now working directly with characters instead of hex
                std::string decryptedText;
                for (size_t i = 0; i < encryptedContent.length(); i++) {
                    // XOR with key at position
                    char decryptedChar = XOREncryptDecryptChar(encryptedContent[i], secretKey, i);
                    decryptedText += decryptedChar;
                }

                // Display decrypted content
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << "\nDecrypted content from " << decryptFilename << ":\n" << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << decryptedText << std::endl << std::endl;

                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << "Press ESC to return to encrypt mode..." << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }
        else if (waitingForFilename) {
            // Process filename input for encryption
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);
            WORD character = 0;

            // Handle backspace for filename
            if (key == VK_BACK && outputFilename.length() > 0) {
                outputFilename.pop_back();
                // Erase the last character
                std::cout << "\b \b";
                return 1;
            }

            // Convert key to ASCII character
            int result = ToAscii(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, &character, 0);
            if (result == 1) {
                char charValue = static_cast<char>(character);
                // Check for valid filename characters (basic check)
                if (isalnum(charValue) || charValue == '.' || charValue == '_' || charValue == '-') {
                    outputFilename += charValue;
                    std::cout << charValue; // Show the character as typed
                }
            }
        }
        else if (waitingForDecryptFilename) {
            // Process filename input for decryption
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);
            WORD character = 0;

            // Handle backspace for filename
            if (key == VK_BACK && decryptFilename.length() > 0) {
                decryptFilename.pop_back();
                // Erase the last character
                std::cout << "\b \b";
                return 1;
            }

            // Convert key to ASCII character
            int result = ToAscii(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, &character, 0);
            if (result == 1) {
                char charValue = static_cast<char>(character);
                // Check for valid filename characters (basic check)
                if (isalnum(charValue) || charValue == '.' || charValue == '_' || charValue == '-') {
                    decryptFilename += charValue;
                    std::cout << charValue; // Show the character as typed
                }
            }
        }
        else if (waitingForDecryptKey) {
            // Process password input in decrypt mode
            // Convert virtual key to character
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);
            char buffer[2] = {0};
            WORD character = 0;

            // Handle backspace
            if (key == VK_BACK && tempKey.length() > 0) {
                tempKey.pop_back();
                // Erase the last asterisk
                std::cout << "\b \b";
                return 1;
            }

            // Convert key to ASCII character
            int result = ToAscii(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), keyboardState, &character, 0);
            if (result == 1) {
                // Add character to tempKey
                tempKey += (char)character;
                std::cout << '*'; // Display '*' instead of the actual character
            }
        }
        else if (!decryptMode) {
            // Normal encrypt mode operation
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
                        // Use a reasonable key length to avoid memory issues
                        secretKey = GeneratePassword(100, secretKey);
                    }
                } else {
                    if (!paddingAdded) {
                        std::string padding = GeneratePadding(100); // Generate reasonable padding
                        encryptedBuffer += padding; // Add padding to encrypted buffer
                        paddingAdded = true;
                    }
                    // Updated: direct character encryption without hex conversion
                    char encryptedChar = XOREncryptDecryptChar(buffer[0], secretKey, encryptedBuffer.size());
                    std::cout << encryptedChar; // Show encrypted character (might show non-printable chars)
                    encryptedBuffer += encryptedChar; // Add encrypted character to encrypted buffer
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
    std::cout << "Please enter output filename: ";

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
    // Close the output file
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

void DumpToFile(const std::string& text) {
    if (outputFile.is_open()) {
        outputFile << text << std::endl;
    } else {
        std::cerr << "Failed to write to file. Output file is not open." << std::endl;
    }
}

void EnterDecryptMode() {
    decryptMode = true;
    waitingForDecryptFilename = true;
    waitingForDecryptKey = false;
    tempKey = "";
    decryptFilename = "";

    system("cls");
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::cout << "=================================================" << std::endl;
    std::cout << "                DECRYPT MODE ACTIVE               " << std::endl;
    std::cout << "=================================================" << std::endl;

    // First ask for the filename to decrypt
    std::cout << "Enter filename to decrypt (press Enter for default 'output.txt'): ";
    // Filename entry is now handled by the keyboard hook
}

// New function that directly encrypts/decrypts a character without hex conversion
char XOREncryptDecryptChar(char input, const std::string& key, size_t pos) {
    return input ^ key[pos % key.length()];
}
