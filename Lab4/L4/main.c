#include <stdio.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "errors.h"

#define MEMORY_SIZE 1024 * 1024

typedef struct Allocator Allocator;

typedef Allocator *create_allocator_func(void *memory, size_t size);

typedef void *allocator_alloc_func(Allocator *const allocator, const size_t size);

typedef void allocator_free_func(Allocator *const allocator, void *const memory);

typedef void allocator_destroy_func(Allocator *const allocator);

static create_allocator_func *create_allocator;
static allocator_alloc_func *allocator_alloc;
static allocator_free_func *allocator_free;
static allocator_destroy_func *allocator_destroy;

int print_error(error_msg error) {
    char buffer[100];
    if (error.type) {
        snprintf(buffer, 100, "Error - %s: %s\n", error.func, error.msg);
        write(STDERR_FILENO, buffer, strlen(buffer));
        return error.type;
    }
    return 0;
}

error_msg init_library(void *library) {
    create_allocator = dlsym(library, "allocator_create");
    if (create_allocator == NULL) {
        dlclose(library);
        return (error_msg) {INCORRECT_OPTIONS_ERROR, "main", "failed to find create function"};
    }

    allocator_alloc = dlsym(library, "allocator_alloc");
    if (allocator_alloc == NULL) {
        dlclose(library);
        return (error_msg) {INCORRECT_OPTIONS_ERROR, "main", "failed to find alloc function"};
    }

    allocator_free = dlsym(library, "allocator_free");
    if (allocator_free == NULL) {
        dlclose(library);
        return (error_msg) {INCORRECT_OPTIONS_ERROR, "main", "failed to find free function"};
    }

    allocator_destroy = dlsym(library, "allocator_destroy");
    if (allocator_destroy == NULL) {
        dlclose(library);
        return (error_msg) {INCORRECT_OPTIONS_ERROR, "main", "failed to find destroy function"};
    }
    return (error_msg) {SUCCESS, "", ""};
}


int main(int argc, char **argv) {
    void *library = NULL;

    if (argc == 2) {
        library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
    }
    if (argc != 2 || library == NULL) {
        library = dlopen("./libfree-block-allocator.so", RTLD_GLOBAL | RTLD_LAZY);
    }
    if (library == NULL) {
        return print_error((error_msg) {INCORRECT_OPTIONS_ERROR, "main", "incorrect count args"});
    }
    void *memory = mmap(
            NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );

    if (memory == MAP_FAILED) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "map"});
    }

    error_msg errorMsg = init_library(library);
    if(errorMsg.type){
        dlclose(library);
        return print_error(errorMsg);
    }

    Allocator * allocator = create_allocator(memory, MEMORY_SIZE);
    if(allocator == NULL){
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "allocator didn't create"});
    }

    // Тест 1: Выделение и освобождение памяти
    printf("Test 1: Allocating and freeing memory...\n");
    int *a = allocator_alloc(allocator, sizeof(int) * 10);
    if (a == NULL) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "memory allocated"});
    }
    a[0] = 13;
    a[9] = 19;
    printf("a[0] = %d, a[9] = %d\n", a[0], a[9]);
    allocator_free(allocator, a);
    printf("Test 1 passed.\n\n");

    // Тест 2: Выделение памяти большего размера
    printf("Test 2: Allocating larger memory block...\n");
    int *b = allocator_alloc(allocator, sizeof(int) * 1000);
    if (b == NULL) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "memory allocated"});
    }
    b[0] = 42;
    b[999] = 24;
    printf("b[0] = %d, b[999] = %d\n", b[0], b[999]);
    allocator_free(allocator, b);
    printf("Test 2 passed.\n\n");

    // Тест 3: Повторное выделение памяти после освобождения
    printf("Test 3: Reallocating memory after freeing...\n");
    int *c = allocator_alloc(allocator, sizeof(int) * 5);
    if (c == NULL) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "memory allocated"});
    }
    c[0] = 7;
    c[4] = 14;
    printf("c[0] = %d, c[4] = %d\n", c[0], c[4]);
    allocator_free(allocator, c);
    printf("Test 3 passed.\n\n");

    // Тест 4: Попытка выделения слишком большого блока памяти
    printf("Test 4: Attempting to allocate too much memory...\n");
    int *d = allocator_alloc(allocator, MEMORY_SIZE + 1); // Попытка выделить больше, чем доступно
    if (d != NULL) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "memory allocated unexpectedly"});
    }
    printf("Test 4 passed (expected failure).\n\n");

    // Тест 5: Выделение нескольких блоков памяти
    printf("Test 5: Allocating multiple memory blocks...\n");
    int *e = allocator_alloc(allocator, sizeof(int) * 10);
    int *f = allocator_alloc(allocator, sizeof(int) * 20);
    if (e == NULL || f == NULL) {
        return print_error((error_msg) {MEMORY_ALLOCATED_ERROR, "main", "memory allocated"});
    }
    e[0] = 1;
    f[0] = 2;
    printf("e[0] = %d, f[0] = %d\n", e[0], f[0]);
    allocator_free(allocator, e);
    allocator_free(allocator, f);
    printf("Test 5 passed.\n\n");

    allocator_destroy(allocator);

    dlclose(library);
    return 0;
}