#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

#define INT_SIZE 32
typedef struct thread_data
{
    int* array;
    int min;
    int max;
    int start;
    int end;
} thread_data;

void* find_min_max(void* arg);

void print_array(int* arr);
void* find_min_max(void* arg);

// Мьютекс для синхронизации доступа к global_min и global_max
pthread_mutex_t mutex;

int global_min, global_max;

int main(int argc, char** argv)
{
    if (argc != 4) {
        const char msg[] = "Usage: %s <array_size> <max_threads> <seed>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_SUCCESS);
    }

    long arr_size =  atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    //unsigned int seed = atoi(argv[3]);

    if (arr_size <= 0 || max_threads <= 0) {
        const char msg[] = "Error: Array size and max threads must be positive integers.\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_SUCCESS);
    }

    int* array = (int*) malloc(arr_size * sizeof(int));
    for (long i = 0; i < arr_size; i++)
        array[i] = rand() % 1000;

    pthread_t threads[max_threads];
    thread_data thread_data_arr[max_threads];

    int chunk_size = arr_size / max_threads + (arr_size % max_threads != 0);

    // Инициализация мьютекса
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }

    global_min = array[0];
    global_max = array[0];

    clock_t start_full, end_full, start_creation, end_creation;

    start_full = clock();
    for (int i = 0; i < max_threads; i++) {
        thread_data_arr[i].array = array;
        thread_data_arr[i].start = i * chunk_size;
        thread_data_arr[i].end = (i == max_threads - 1) ? arr_size : (i + 1) * chunk_size;

        start_creation = clock();
        if (pthread_create(&threads[i], NULL, find_min_max, &thread_data_arr[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        end_creation = clock();

        printf("Creation time of %d: %f seconds\n", i, (double)(end_creation - start_creation) / CLOCKS_PER_SEC);
    }

    clock_t start_join = clock();
    for (int i = 0; i < max_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(EXIT_FAILURE);
        }
    }
    clock_t end_join = clock();

    end_full = clock();

    printf("Full time: %f seconds\n", (double)(end_full - start_full) / CLOCKS_PER_SEC);
    printf("Join time: %f seconds\n", (double)(end_join - start_join) / CLOCKS_PER_SEC);

    {
        char buff[INT_SIZE];
        int len = snprintf(buff, sizeof(buff), "Global Min: %d\n", global_min);
        write(STDOUT_FILENO, buff, len);
    }

    {
        char buff[INT_SIZE];
        int len = snprintf(buff, sizeof(buff), "Global Max: %d\n", global_max);
        write(STDOUT_FILENO, buff, len);
    }

    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);

    free(array);

    return 0;
}

void* find_min_max(void* arg)
{
    clock_t start = clock();

    thread_data* data = (thread_data*) arg;

    int local_min = data->array[data->start];
    int local_max = data->array[data->start];

    for (int i = data->start; i < data->end; i++) {
        if (data->array[i] > local_max)
            local_max = data->array[i];

        if (data->array[i] < local_min)
            local_min = data->array[i];
    }

    // Защита глобальных переменных с помощью мьютекса
    pthread_mutex_lock(&mutex);
    if (local_min < global_min)
        global_min = local_min;
    if (local_max > global_max)
        global_max = local_max;
    pthread_mutex_unlock(&mutex);

    clock_t end = clock();

    printf("Function time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return NULL;
}

void print_array(int* arr)
{
    const char msg[] = "Array:\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while(*arr) {
        char buff[INT_SIZE + 1];
        int len = snprintf(buff, sizeof(buff), "%d\n", *arr);
        write(STDOUT_FILENO, buff, len);
        arr++;
    }
}