#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SIZE_BUF 100
#define SIZE_MSG 100
#define SIZE_CMDLINE 256

void HandleError(const char *message) {
    DWORD errorCode = GetLastError();
    char errorBuffer[SIZE_MSG];
    snprintf(errorBuffer, SIZE_MSG, "%s (error code: %lu)\n", message, errorCode);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), errorBuffer, strlen(errorBuffer), NULL, NULL);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    HANDLE pipeRead, pipeWrite;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    char buffer[SIZE_BUF];

    if (argc < 2) {
        const char error_msg[] = "You must specify a file name as an argument.\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error_msg, sizeof(error_msg) - 1, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&pipeRead, &pipeWrite, &sa, 0)) {
        HandleError("Failed to create pipe");
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdInput = pipeRead;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    char cmdLine[SIZE_CMDLINE];
    size_t len = strlen(argv[1]);
    if (len >= sizeof(cmdLine) - 11) {
        const char error_msg[] = "Argument is too long\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), error_msg, sizeof(error_msg) - 1, NULL, NULL);
        CloseHandle(pipeRead);
        CloseHandle(pipeWrite);
        exit(EXIT_FAILURE);
    }

    strcpy(cmdLine, "child.exe ");
    strcat(cmdLine, argv[1]);

    if (!CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        HandleError("Failed to create process");
        CloseHandle(pipeRead);
        CloseHandle(pipeWrite);
    }

    CloseHandle(pipeRead);

    while (1) {
        const char prompt[] = "Enter numbers (or 'end' to finish): ";
        if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), prompt, sizeof(prompt) - 1, NULL, NULL)) {
            HandleError("Failed to write prompt");
        }

        DWORD bytesRead;
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            HandleError("Failed to read input");
        }

        buffer[bytesRead] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strcmp(buffer, "end") == 0) {
            DWORD written;
            if (!WriteFile(pipeWrite, "end\n", 4, &written, NULL)) {
                HandleError("Failed to write 'end' to pipe");
            }
            CloseHandle(pipeWrite);
            break;
        }


        DWORD written;
        if (!WriteFile(pipeWrite, buffer, strlen(buffer), &written, NULL) ||
            !WriteFile(pipeWrite, "\n", 1, &written, NULL)) {
            HandleError("Failed to write to pipe");
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
