#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_WRITE "/sem_write"
#define SEM_READ "/sem_read"
#define BUFFER_SIZE 100

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Необходимо указать имя файла в качестве аргумента.\n");
        exit(EXIT_FAILURE);
    }

    // Удаляем старые ресурсы, если они существуют
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_WRITE);
    sem_unlink(SEM_READ);

    // Создаем и настраиваем общую память
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    char *shared_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Создаем семафоры
    sem_t *sem_write = sem_open(SEM_WRITE, O_CREAT | O_EXCL, 0644, 1);
    sem_t *sem_read = sem_open(SEM_READ, O_CREAT | O_EXCL, 0644, 0);
    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED) {
        perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_WRITE);
        sem_unlink(SEM_READ);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        munmap(shared_memory, BUFFER_SIZE);
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_WRITE);
        sem_unlink(SEM_READ);
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Дочерний процесс
        char buffer[BUFFER_SIZE];
        while (1) {
            // Ожидание разрешения на чтение от родителя
            sem_wait(sem_read);

            // Проверяем, есть ли "end"
            if (strcmp(shared_memory, "end") == 0) {
                break;
            }

            // Чтение из общей памяти
            strncpy(buffer, shared_memory, BUFFER_SIZE);
            buffer[BUFFER_SIZE - 1] = '\0';

            // Вывод для проверки
            printf("[Child] Прочитано из общей памяти: %s\n", buffer);

            // Разрешаем родителю записывать
            sem_post(sem_write);
        }

        // Завершаем работу
        munmap(shared_memory, BUFFER_SIZE);
        sem_close(sem_write);
        sem_close(sem_read);
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        char input[BUFFER_SIZE];
        while (1) {
            printf("Введите числа (или end для завершения): ");
            fgets(input, BUFFER_SIZE, stdin);
            input[strcspn(input, "\n")] = '\0'; // Убираем символ новой строки

            // Ожидание разрешения на запись
            sem_wait(sem_write);

            // Пишем в общую память
            strncpy(shared_memory, input, BUFFER_SIZE);

            // Разрешаем дочернему процессу читать
            sem_post(sem_read);

            if (strcmp(input, "end") == 0) {
                break;
            }
        }

        // Ожидаем завершения дочернего процесса
        wait(NULL);

        // Освобождаем ресурсы
        munmap(shared_memory, BUFFER_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(sem_write);
        sem_close(sem_read);
        sem_unlink(SEM_WRITE);
        sem_unlink(SEM_READ);
    }

    return 0;
}
