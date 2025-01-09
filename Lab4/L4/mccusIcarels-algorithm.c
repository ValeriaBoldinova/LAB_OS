#include "mccusIcarels-algorithm.h"

Allocator *allocator_create(void *const memory, const size_t size) {
    if (memory == NULL || size < PAGE_SIZE) {
        return NULL;
    }

    Allocator *allocator = (Allocator *) memory;
    allocator->memory = (uint8_t *) memory + sizeof(Allocator);
    allocator->size = size - sizeof(Allocator);
    allocator->free_pages = (Page *) allocator->memory;

    size_t num_pages = allocator->size / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        Page *page = (Page *) ((uint8_t *) allocator->memory + i * PAGE_SIZE);
        page->page_size = PAGE_SIZE;
        page->next_free = (i == num_pages - 1) ? NULL : (Page *) ((uint8_t *) allocator->memory + (i + 1) * PAGE_SIZE);
        page->free_blocks = NULL;
    }

    return allocator;
}


void allocator_destroy(Allocator *const allocator) {
    if (allocator == NULL) {
        return;
    }

    munmap(allocator, allocator->size + sizeof(Allocator));
}

void *allocator_alloc(Allocator *const allocator, const size_t size) {
    if (allocator == NULL || size == 0 || size > allocator->size) {
        return NULL;
    }

    size_t block_size = 1;
    while (block_size < size + sizeof(Block) && block_size < PAGE_SIZE) {
        block_size *= 2;
    }

    Page *page = allocator->free_pages;
    while (page != NULL) {
        Block *block = page->free_blocks;
        Block *prev = NULL;

        while (block != NULL) {
            if (block->block_size >= block_size && block->is_free) {
                if (prev == NULL) {
                    page->free_blocks = block->next_free;
                } else {
                    prev->next_free = block->next_free;
                }
                block->is_free = false;
                return (void *) ((uint8_t *) block + sizeof(Block));
            }
            prev = block;
            block = block->next_free;
        }
        page = page->next_free;
    }

    page = allocator->free_pages;
    if (page == NULL) {
        return NULL;
    }
    allocator->free_pages = page->next_free;

    size_t num_blocks = PAGE_SIZE / block_size;
    for (size_t i = 0; i < num_blocks; i++) {
        Block *block = (Block *) ((uint8_t *) page + i * block_size);
        block->block_size = block_size;
        block->next_free = page->free_blocks;
        block->is_free = true;
        page->free_blocks = block;
    }

    Block *block = page->free_blocks;
    page->free_blocks = block->next_free;
    block->is_free = false;
    return (void *) ((uint8_t *) block + sizeof(Block));
}

void allocator_free(Allocator *const allocator, void *const memory) {
    if (allocator == NULL || memory == NULL) {
        return;
    }

    Block *block = (Block *) ((uint8_t *) memory - sizeof(Block));

    Page *page = allocator->free_pages;
    while (page != NULL) {
        if ((uint8_t *) block >= (uint8_t *) page && (uint8_t *) block < (uint8_t *) page + PAGE_SIZE) {
            block->next_free = page->free_blocks;
            block->is_free = true;
            page->free_blocks = block;
            return;
        }
        page = page->next_free;
    }
}