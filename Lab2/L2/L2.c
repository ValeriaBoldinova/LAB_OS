#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#define INT_SIZE 32

// Структура для передачи данных в поток
typedef struct {
    int *array;
    int start;
    int end;
} thread_data;

// Глобальные переменные для хранения минимального и максимального значений
pthread_mutex_t mutex;
int global_min;
int global_max;

void *find_min_max(void *arg);

int format_output(char *buffer, size_t size, const char *format, ...);

int simple_itoa(char *buffer, int value);

int main(int argc, char **argv) {
    if (argc != 4) {
        const char usage_msg[] = "Usage: <program_name> <array_size> <max_threads> <seed>\n";
        write(STDERR_FILENO, usage_msg, sizeof(usage_msg) - 1);
        return EXIT_FAILURE;
    }

    int arr_size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    int seed = atoi(argv[3]);

    if (arr_size <= 0 || max_threads <= 0) {
        const char error_msg[] = "Error: Array size and max threads must be positive integers.\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        return EXIT_FAILURE;
    }

    // Инициализация массива
    srand(seed);
    int *array = malloc(arr_size * sizeof(int));
    if (!array) {
        const char alloc_fail_msg[] = "Failed to allocate memory\n";
        write(STDERR_FILENO, alloc_fail_msg, sizeof(alloc_fail_msg) - 1);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < arr_size; i++) {
        array[i] = rand() % 1000;
    }

    // Инициализация мьютекса
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        const char mutex_fail_msg[] = "Failed to initialize mutex\n";
        write(STDERR_FILENO, mutex_fail_msg, sizeof(mutex_fail_msg) - 1);
        free(array);
        return EXIT_FAILURE;
    }

    // Инициализация глобальных переменных
    global_min = array[0];
    global_max = array[0];

    pthread_t threads[max_threads];
    thread_data thread_data_arr[max_threads];

    int chunk_size = (arr_size + max_threads - 1) / max_threads; // Округление вверх

    clock_t start_time = clock();

    // Создание потоков
    for (int i = 0; i < max_threads; i++) {
        thread_data_arr[i].array = array;
        thread_data_arr[i].start = i * chunk_size;
        thread_data_arr[i].end = (i + 1) * chunk_size;
        if (thread_data_arr[i].end > arr_size) {
            thread_data_arr[i].end = arr_size;
        }

        if (pthread_create(&threads[i], NULL, find_min_max, &thread_data_arr[i]) != 0) {
            const char thread_fail_msg[] = "Failed to create thread\n";
            write(STDERR_FILENO, thread_fail_msg, sizeof(thread_fail_msg) - 1);
            free(array);
            pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < max_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            const char join_fail_msg[] = "Failed to join thread\n";
            write(STDERR_FILENO, join_fail_msg, sizeof(join_fail_msg) - 1);
            free(array);
            pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }
    }

    clock_t end_time = clock();

    // Вывод результатов
    char buffer[128];
    int len = 0;
    len += format_output(buffer + len, sizeof(buffer) - len, "Global Min: %d\n", global_min);
    len += format_output(buffer + len, sizeof(buffer) - len, "Global Max: %d\n", global_max);
    len += format_output(buffer + len, sizeof(buffer) - len, "Execution Time: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);
    write(STDOUT_FILENO, buffer, len);

    // Освобождение ресурсов
    free(array);
    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}

void *find_min_max(void *arg) {
    thread_data *data = (thread_data *)arg;
    int local_min = data->array[data->start];
    int local_max = data->array[data->start];

    for (int i = data->start; i < data->end; i++) {
        if (data->array[i] < local_min) {
            local_min = data->array[i];
        }
        if (data->array[i] > local_max) {
            local_max = data->array[i];
        }
    }

    pthread_mutex_lock(&mutex);
    if (local_min < global_min) {
        global_min = local_min;
    }
    if (local_max > global_max) {
        global_max = local_max;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int format_output(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);

    const char *traverse = format;
    char *buf_ptr = buffer;
    size_t remaining = size;

    while (*traverse && remaining > 1) {
        if (*traverse == '%') {
            traverse++;
            if (*traverse == 'd') {
                int value = va_arg(args, int);
                char temp[INT_SIZE];
                int len = simple_itoa(temp, value);
                if (len < remaining) {
                    memcpy(buf_ptr, temp, len);
                    buf_ptr += len;
                    remaining -= len;
                }
            } else if (*traverse == 'f') {
                double value = va_arg(args, double);
                int int_part = (int)value;
                double frac_part = value - int_part;
                char temp[INT_SIZE];
                int len = simple_itoa(temp, int_part);
                if (len < remaining) {
                    memcpy(buf_ptr, temp, len);
                    buf_ptr += len;
                    remaining -= len;
                }
                if (remaining > 2) {
                    *buf_ptr++ = '.';
                    remaining--;
                    for (int i = 0; i < 6 && remaining > 1; i++) {
                        frac_part *= 10;
                        int digit = (int)frac_part;
                        *buf_ptr++ = '0' + digit;
                        frac_part -= digit;
                        remaining--;
                    }
                }
            }
        } else {
            *buf_ptr++ = *traverse;
            remaining--;
        }
        traverse++;
    }

    *buf_ptr = '\0';
    va_end(args);

    return buf_ptr - buffer;
}

int simple_itoa(char *buffer, int value) {
    char temp[INT_SIZE];
    int i = 0;
    int is_negative = value < 0;

    if (is_negative) {
        value = -value;
    }

    do {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);

    if (is_negative) {
        temp[i++] = '-';
    }

    int len = i;
    for (int j = 0; j < i; j++) {
        buffer[j] = temp[i - j - 1];
    }
    buffer[len] = '\0';

    return len;
}
