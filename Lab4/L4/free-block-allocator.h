#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

typedef struct Block {
    size_t size;
    struct Block *next;
    int is_free;
} Block;

typedef struct {
    Block *free_list;
    void *memory;
    size_t size;
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);

void *allocator_alloc(Allocator *allocator, size_t size);

void allocator_free(Allocator *allocator, void *ptr);

void allocator_destroy(Allocator *allocator);