#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <semaphore.h> 

#define BUFFER_SIZE 64

typedef struct task_data {
    int* numbers;
    int range_start;
    int range_end;
} task_data;

int shared_min;
int shared_max;
sem_t active_threads;  // Семафор для ограничения потоков
pthread_mutex_t min_max_mutex = PTHREAD_MUTEX_INITIALIZER;  // Мьютекс для защиты глобальных значений

void* process_range(void* arg);
void output_number(int fd, double number);  // Для вывода времени
void output_int(int fd, int number);  // Для вывода целых чисел

int main(int argc, char** argv) {
    if (argc != 4) {
        const char usage_msg[] = "Usage: ./program <array_length> <max_active_threads> <random_seed>\n";
        write(STDERR_FILENO, usage_msg, sizeof(usage_msg) - 1);
        _exit(EXIT_FAILURE);
    }

    long array_length = atol(argv[1]);
    int max_active_threads = atoi(argv[2]);
    unsigned int random_seed = atoi(argv[3]);

    if (array_length <= 0 || max_active_threads <= 0) {
        const char error_msg[] = "Error: Array length and max threads must be positive integers.\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        _exit(EXIT_FAILURE);
    }

    // Выделение памяти под массив
    int* data_array = malloc(array_length * sizeof(int));
    if (!data_array) {
        const char error_msg[] = "Memory allocation failed for the array\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        _exit(EXIT_FAILURE);
    }

    srand(random_seed);
    for (long i = 0; i < array_length; i++) {
        data_array[i] = rand() % 1000;  // Заполнение массива случайными числами
    }

    pthread_t* thread_pool = malloc(max_active_threads * sizeof(pthread_t));
    task_data* tasks = malloc(max_active_threads * sizeof(task_data));

    sem_init(&active_threads, 0, max_active_threads);

    shared_min = data_array[0];
    shared_max = data_array[0];

    struct timeval program_start, program_end;  // Структуры для времени в секундах
    gettimeofday(&program_start, NULL);  // Засекаем время начала

    int chunk_size = array_length / max_active_threads + (array_length % max_active_threads != 0);
    int total_threads = 0;

    for (int i = 0; i < max_active_threads; i++) {
        tasks[i].numbers = data_array;
        tasks[i].range_start = i * chunk_size;
        tasks[i].range_end = (i == max_active_threads - 1) ? array_length : (i + 1) * chunk_size;

        sem_wait(&active_threads);

        if (pthread_create(&thread_pool[i], NULL, process_range, &tasks[i]) != 0) {
            const char error_msg[] = "Thread creation failed\n";
            write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
            free(data_array);
            _exit(EXIT_FAILURE);
        }
        total_threads++;
    }

    for (int i = 0; i < total_threads; i++) {
        if (pthread_join(thread_pool[i], NULL) != 0) {
            const char error_msg[] = "Thread joining failed\n";
            write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
            free(data_array);
            _exit(EXIT_FAILURE);
        }
    }

    gettimeofday(&program_end, NULL);  // Засекаем время окончания

    // Вычисляем время в секундах
    double exec_time_seconds = (program_end.tv_sec - program_start.tv_sec) +
                               (program_end.tv_usec - program_start.tv_usec) / 1000000.0;

    const char exec_time_msg[] = "Execution time: ";
    write(STDOUT_FILENO, exec_time_msg, sizeof(exec_time_msg) - 1);
    output_number(STDOUT_FILENO, exec_time_seconds);  // Выводим время в секундах
    const char newline[] = " seconds\n";
    write(STDOUT_FILENO, newline, sizeof(newline) - 1);

    const char min_label[] = "Minimum value: ";
    write(STDOUT_FILENO, min_label, sizeof(min_label) - 1);
    output_int(STDOUT_FILENO, shared_min);  // Используем новую функцию для целых чисел
    write(STDOUT_FILENO, "\n", 1);

    const char max_label[] = "Maximum value: ";
    write(STDOUT_FILENO, max_label, sizeof(max_label) - 1);
    output_int(STDOUT_FILENO, shared_max);  // Используем новую функцию для целых чисел
    write(STDOUT_FILENO, "\n", 1);

    sem_destroy(&active_threads);
    free(data_array);
    free(thread_pool);
    free(tasks);

    return 0;
}

void* process_range(void* arg) {
    task_data* task = (task_data*)arg;

    int local_min = task->numbers[task->range_start];
    int local_max = task->numbers[task->range_start];

    for (int i = task->range_start; i < task->range_end; i++) {
        if (task->numbers[i] < local_min)
            local_min = task->numbers[i];
        if (task->numbers[i] > local_max)
            local_max = task->numbers[i];
    }

    // Используем мьютекс для безопасного обновления глобальных значений
    pthread_mutex_lock(&min_max_mutex);
    if (local_min < shared_min) {
        shared_min = local_min;
    }
    if (local_max > shared_max) {
        shared_max = local_max;
    }
    pthread_mutex_unlock(&min_max_mutex);

    sem_post(&active_threads);
    return NULL;
}

void output_number(int fd, double number) {
    char buffer[BUFFER_SIZE];
    int length = snprintf(buffer, sizeof(buffer), "%.6f", number);  // Вывод с 6 знаками после запятой
    write(fd, buffer, length);
}

// Новая функция для вывода целых чисел
void output_int(int fd, int number) {
    char buffer[BUFFER_SIZE];
    int length = snprintf(buffer, sizeof(buffer), "%d", number);  // Выводим как целое число
    write(fd, buffer, length);
}
