#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#define SEM_NAME "/my_semaphore"
#define SHM_NAME "/my_shared_memory"

int main(int argc, char *argv[]) {
    char buffer[4096];
    char *token;
    float sum = 0.0f;

    if (argc < 2) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Необходимо указать имя файла в качестве аргумента.\n");
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];

    // Открываем семафор
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Открываем общую память
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    int *shared_memory = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; // Убираем символ новой строки
        sum = 0.0f;
        token = strtok(buffer, " ");
        while (token != NULL) {
            sum += atof(token);
            token = strtok(NULL, " ");
        }

        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char sum_str[50];
        snprintf(sum_str, sizeof(sum_str), "%.2f\n", sum);
        write(fd, sum_str, strlen(sum_str));
        close(fd);

        // Обновляем сумму в общей памяти
        sem_wait(sem);
        *shared_memory += (int)sum; // Сохраняем целую часть суммы
        sem_post(sem);
    }

    sem_close(sem);
    munmap(shared_memory, sizeof(int));
    close(shm_fd);

    return 0;
}
