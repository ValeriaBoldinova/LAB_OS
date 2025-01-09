#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

typedef struct Block {
    size_t block_size;
    struct Block *next_free;
    bool is_free;
} Block;

typedef struct Page {
    size_t page_size;
    struct Page *next_free;
    Block *free_blocks;
} Page;

typedef struct Allocator {
    void *memory;
    size_t size;
    Page *free_pages;
} Allocator;


Allocator *allocator_create(void *const memory, const size_t size);

void *allocator_alloc(Allocator *const allocator, const size_t size);

void allocator_free(Allocator *const allocator, void *const memory);

void allocator_destroy(Allocator *const allocator);