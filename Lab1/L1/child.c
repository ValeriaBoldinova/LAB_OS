#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define SIZE_BUF 4096
#define SIZE_MSG 128

//Функция для обработки ошибок и завершения программы
void HandleError(const char *message) {
    const char error_msg[] = "Error: "; // "Ошибка: "
    write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
    write(STDERR_FILENO, message, strlen(message));
    write(STDERR_FILENO, "\n", 1);
    exit(EXIT_FAILURE);
}

//Функция для записи суммы в файл
void writeSumToFile(const char *filename, float sum) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        HandleError("opening the file"); // "открытие файла"
    }

    char sum_str[20];
    int len = snprintf(sum_str, sizeof(sum_str), "%.2f\n", sum); // Output with 2 decimal places (Вывод с точностью до 2 знаков)
    if (len < 0 || write(fd, sum_str, len) != len) {
        close(fd);
        HandleError("writing to the file"); // "запись в файл"
    }

    if (close(fd) == -1) {
        HandleError("closing the file"); // "закрытие файла"
    }
}

int main(int argc, char *argv[]) {
    char buffer[SIZE_BUF];

    // Check if a filename is provided as an argument (Проверяем, передано ли имя файла как аргумент)
    if (argc < 2) {
        const char error_msg[] = "You must specify a file name as an argument.\n"; // "Вы должны указать имя файла как аргумент"
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];

    while (1) {
        // Читаем ввод от родительского процесса
        ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytesRead == -1) {
            HandleError("reading input"); // "чтение входных данных"
        }

        if (bytesRead == 0) { // (EOF) Обнаружен конец ввода
            break;
        }

        buffer[bytesRead] = '\0';
        buffer[strcspn(buffer, "\n")] = 0; // Удаляем символ новой строки

        if (strlen(buffer) == 0) {
            continue; // Skip empty input Пропускаем пустой ввод)

        }

        if (strcmp(buffer, "end") == 0) {
           break;
        }


        char *token;
        float sum = 0.0f;
        char *endptr;

        // Parse and process input tokens (Разбираем и обрабатываем токены ввода)
        token = strtok(buffer, " ");
        while (token != NULL) {
            errno = 0;
            float num = strtof(token, &endptr); // Use strtof for float (Используем strtof для работы с float)

            if (errno != 0 || *endptr != '\0') {
                const char error_msg[] = "Invalid number in input. Skipping.\n"; // "Недопустимое число во входных данных. Пропуск."
                write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
            } else {
                sum += num;
            }

            token = strtok(NULL, " ");
        }

        // Write the computed sum to the file (Записываем вычисленную сумму в файл)
        writeSumToFile(filename, sum);
    }

    return 0;
}
