#include "free-block-allocator.h"


Allocator *allocator_create(void *const memory, const size_t size) {
    if (memory == NULL) {
        return NULL;
    }


    Allocator* allocator = (Allocator*)memory;
    allocator->memory = (void *) ((char *) memory + sizeof(Allocator));
    allocator->size = size - sizeof(Allocator);
    allocator->free_list = (Block*)allocator->memory;
    allocator->free_list->size = allocator->size - sizeof(Block);
    allocator->free_list->next = NULL;
    allocator->free_list->is_free = 1;

    return allocator;
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if(allocator == NULL){
        return NULL;
    }
    if(size > allocator->size){
        return NULL;
    }
    Block *curr = allocator->free_list;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(Block) + 1) {
                Block *new_block = (Block *) ((char *) curr + sizeof(Block) + size);
                new_block->size = curr->size - size - sizeof(Block);
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->size = size;
                curr->next = new_block;
            }

            curr->is_free = 0;
            return (void *) (curr + 1);
        }
        curr = curr->next;
    }

    return NULL;
}

void allocator_free(Allocator *allocator, void *ptr) {
    if (ptr == NULL) return;

    Block *block = (Block *) ptr - 1;
    block->is_free = 1;


    Block *curr = allocator->free_list;
    while (curr != NULL && curr->next != NULL) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(Block) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void allocator_destroy(Allocator* allocator) {
    if (munmap(allocator, allocator->size + sizeof(Allocator)) == -1) {
        perror("munmap failed");
    }
}