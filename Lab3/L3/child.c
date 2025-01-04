#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define BUFFER_SIZE 256
#define SHARED_MEMORY_NAME "Local\\MySharedMemory"
#define EMPTY_MARKER "EMPTY"  // Marker for empty memory

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];

    // Open the shared memory
    HANDLE hMapFile = OpenFileMapping(
            FILE_MAP_READ | FILE_MAP_WRITE,  // Read and write access
            FALSE,                          // Do not inherit handle
            SHARED_MEMORY_NAME              // Name of the shared memory
    );
    if (hMapFile == NULL) {
        fprintf(stderr, "Failed to open shared memory. Error code: %lu\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    // Map the shared memory into the address space of this process
    char *lpBaseAddress = (char *)MapViewOfFile(
            hMapFile,                      // Handle to the shared memory
            FILE_MAP_READ | FILE_MAP_WRITE, // Read and write access
            0,                             // Offset (high-order DWORD)
            0,                             // Offset (low-order DWORD)
            BUFFER_SIZE                    // Number of bytes to map
    );
    if (lpBaseAddress == NULL) {
        fprintf(stderr, "Failed to map shared memory. Error code: %lu\n", GetLastError());
        CloseHandle(hMapFile);
        exit(EXIT_FAILURE);
    }

    // Open the output file in write mode to overwrite any existing content
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        UnmapViewOfFile(lpBaseAddress);
        CloseHandle(hMapFile);
        exit(EXIT_FAILURE);
    }

    // Main loop: read from shared memory and process data
    while (1) {
        // Wait for new data in shared memory
        if (strcmp(lpBaseAddress, EMPTY_MARKER) == 0) {
            Sleep(100);  // Wait and check again
            continue;
        }

        // Copy data from shared memory
        char buffer[BUFFER_SIZE];
        strncpy(buffer, lpBaseAddress, BUFFER_SIZE);
        buffer[BUFFER_SIZE - 1] = '\0';  // Ensure null-termination

        // Check for exit signal
        if (strcmp(buffer, "end") == 0) {
            break;
        }

        // Process the input string
        float sum = 0;
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            char *endPtr;
            float value = strtof(token, &endPtr);

            if (*endPtr != '\0' && *endPtr != '\n') {
                fprintf(stderr, "Invalid input: '%s' is not a number.\n", token);
            } else {
                sum += value;
            }

            token = strtok(NULL, " ");
        }

        // Write the sum to the file
        fprintf(file, "Sum: %.2f\n", sum);
        fflush(file);

        // Mark shared memory as empty
        strncpy(lpBaseAddress, EMPTY_MARKER, BUFFER_SIZE);
    }

    // Cleanup
    fclose(file);
    UnmapViewOfFile(lpBaseAddress);
    CloseHandle(hMapFile);

    return 0;
}

