#include <windows.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 256
#define SHARED_MEMORY_NAME "Local\\MySharedMemory"

void HandleError(const char *message) {
    char errorMsg[BUFFER_SIZE];
    DWORD written;

    // Form error message
    DWORD errorCode = GetLastError();
    int len = wsprintf(errorMsg, "%s. Error code: %lu\n", message, errorCode);

    // Write error message to console
    WriteConsole(GetStdHandle(STD_ERROR_HANDLE), errorMsg, len, &written, NULL);
    ExitProcess(EXIT_FAILURE);
}

void WriteMessage(const char *message) {
    DWORD written;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), message, strlen(message), &written, NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        WriteMessage("Usage: parent.exe <filename>\n");
        return EXIT_FAILURE;
    }

    HANDLE hMapFile;                  // Handle for the file mapping
    LPVOID lpBaseAddress;             // Pointer to the shared memory
    HANDLE hChildProcess;             // Handle for the child process
    PROCESS_INFORMATION pi;           // Process information
    STARTUPINFO si;                   // Startup information
    char filename[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    DWORD written;

    // Use the filename passed as a command line argument
    strncpy(filename, argv[1], BUFFER_SIZE);

    // Create shared memory
    hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,         // Use virtual memory, not a file
            NULL,                         // Default security attributes
            PAGE_READWRITE,               // Read and write access
            0,                            // Maximum size (high-order DWORD)
            BUFFER_SIZE,                  // Maximum size (low-order DWORD)
            SHARED_MEMORY_NAME            // Name of the shared memory
    );
    if (hMapFile == NULL) {
        HandleError("Failed to create shared memory");
    }

    // Map shared memory into the address space of the process
    lpBaseAddress = MapViewOfFile(
            hMapFile,                     // Handle to the shared memory
            FILE_MAP_ALL_ACCESS,          // Read and write access
            0,                            // Offset (high-order DWORD)
            0,                            // Offset (low-order DWORD)
            BUFFER_SIZE                   // Number of bytes to map
    );
    if (lpBaseAddress == NULL) {
        HandleError("Failed to map shared memory");
    }

    // Initialize shared memory with "EMPTY"
    strncpy((char *)lpBaseAddress, "EMPTY", BUFFER_SIZE);

    // Initialize STARTUPINFO for the child process
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    // Create command line string
    char cmdLine[BUFFER_SIZE];
    wsprintf(cmdLine, "child.exe %s", filename);

    // Create child process
    if (!CreateProcess(
            NULL,               // Program name
            cmdLine,            // Command line
            NULL,               // Process security attributes
            NULL,               // Thread security attributes
            FALSE,              // Do not inherit handles
            0,                  // Creation flags
            NULL,               // Environment variables
            NULL,               // Working directory
            &si,                // Startup info
            &pi)) {             // Process information
        HandleError("Failed to create child process");
        UnmapViewOfFile(lpBaseAddress);
        CloseHandle(hMapFile);
    }

    hChildProcess = pi.hProcess;
    CloseHandle(pi.hThread);

    // Main loop: write data to shared memory
    while (1) {
        WriteMessage("Enter numbers separated by spaces (or 'end' to quit): ");
        DWORD read;
        if (!ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buffer, BUFFER_SIZE, &read, NULL)) {
            HandleError("Failed to read input");
            break;
        }
        buffer[read - 2] = '\0';  // Remove the newline character

        // Copy data to shared memory
        strncpy((char *)lpBaseAddress, buffer, BUFFER_SIZE);

        // Exit condition
        if (strcmp(buffer, "end") == 0) {
            break;
        }

        // Wait for child to process the data
        while (strcmp((char *)lpBaseAddress, "EMPTY") != 0) {
            Sleep(100);  // Wait and check again
        }
    }

    // Wait for child process to finish
    WriteMessage("Waiting for the child process to finish...\n");
    WaitForSingleObject(hChildProcess, INFINITE);

    // Cleanup
    UnmapViewOfFile(lpBaseAddress);
    CloseHandle(hMapFile);
    CloseHandle(hChildProcess);

    return 0;
}
