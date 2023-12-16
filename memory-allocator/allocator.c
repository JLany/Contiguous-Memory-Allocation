#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STRATEGY_BEST_FIT 600
#define STRATEGY_WORST_FIT 700
#define STRATEGY_FIRST_FIT 800


struct MemoryBlock {
    char* name;
    unsigned int start;
    unsigned int end;
    unsigned int size;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
};

#define MemoryBlock struct MemoryBlock

void free_mem(MemoryBlock* head) {
    // this is because head is originally in the stack.
    // so no need to free its memory manually.
    head = head->next;

    while (head != NULL) {
        MemoryBlock* temp = head->next;
        free(head->name);
        free(head);
        head = temp;
    }
}

void init_memory(MemoryBlock* m, unsigned int size) {
    m->name = NULL;
    m->start = 0;
    m->end = size - 1;
    m->size = size;
    m->next = NULL;
    m->prev = NULL;
}

void print_stat(MemoryBlock* b) {
    while (b != NULL) {
        if (b->size > 0) {
            printf("Addresses [%d:%d] ", b->start, b->end);

            if (b->name == NULL)
                printf("Unused\n");
            else
                printf("Process %s\n", b->name);
        }

        b = b->next;
    }
}

void split_mem(MemoryBlock* b, unsigned int size, const char* name) {
    MemoryBlock* new = malloc(sizeof(MemoryBlock));

    b->size -= size;
    b->end = b->start + b->size - 1;
    new->next = b->next;

    if (b->next != NULL)
        b->next->prev = new;

    b->next = new;

    new->start = b->end + 1;
    new->end = new->start + size - 1;
    new->size = size;
    new->prev = b;

    new->name = malloc(sizeof(char) * strlen(name) + 1);
    strcpy(new->name, name);
}

int allocatef(MemoryBlock* head, unsigned int size, const char* name) {
    while (head != NULL) {
        if (head->name == NULL && head->size >= size) {
            split_mem(head, size, name);

            return 0;
        }

        head = head->next;
    }

    // did not find enough contiguous memory space.
    return -1;
}

unsigned int size_diff(MemoryBlock* b, unsigned int size) {
    return b->size - size;
}

int allocatew(MemoryBlock* head, unsigned int size, const char* name) {
    MemoryBlock* largest = NULL;
    unsigned int max_diff = __INT32_MAX__;

    while (head != NULL) {
        unsigned int diff = size_diff(head, size);

        if (diff > -1) {
            if (diff > max_diff) {
                max_diff = diff;
                largest = head;
            }
        }

        head = head->next;
    }

    if (largest != NULL) {
        split_mem(largest, size, name);

        return 0;
    }

    return -1;
}

int allocateb(MemoryBlock* head, unsigned int size, const char* name) {
    MemoryBlock* smallest = NULL;
    unsigned int minimum_diff = __INT32_MAX__;

    while (head != NULL) {
        unsigned int diff = size_diff(head, size);

        if (diff > -1) {
            if (diff < minimum_diff) {
                minimum_diff = diff;
                smallest = head;
            }
        }

        head = head->next;
    }

    if (smallest != NULL) {
        split_mem(smallest, size, name);

        return 0;
    }

    return -1;
}

// returns -1 upon faliure.
int allocate_mem(MemoryBlock* head, unsigned int size, const char* name, int strategy) {
    switch (strategy) {
    case STRATEGY_BEST_FIT:
        return allocateb(head, size, name);
    case STRATEGY_FIRST_FIT:
        return allocatef(head, size, name);
    case STRATEGY_WORST_FIT:
        return allocatew(head, size, name);
    default:
        return -1;
    }
}

// merge block with previous/next if they are holes.
void merge_mem(MemoryBlock* b) {
    free(b->name);

    if (b->next != NULL && b->next->name == NULL) {
        MemoryBlock* next = b->next;

        next->start = b->start;
        next->size += b->size;

        b->size = -1;
    }
    else if (b->prev != NULL && b->prev->name == NULL) {
        MemoryBlock* prev = b->prev;

        prev->end = b->end;
        prev->size += b->size;

        b->size = -1;
    }
    else {
        b->name = NULL;
    }

    // Remove because its absorbed by another hole.
    if (b->size == -1) {
        if (b->prev != NULL)
            b->prev->next = b->next;

        if (b->next != NULL)
            b->next->prev = b->prev;

        free(b);
    }
}

int release_mem(MemoryBlock* head, const char* name) {
    while (head != NULL) {
        if (head->name != NULL && strcmp(head->name, name) == 0) {
            merge_mem(head);

            // print_stat(head);
            // printf("%s\n", head->prev->name);
            // print_stat(head->prev);

            return 0;
        }

        head = head->next;
    }

    return -1;
}

// swaps two consecutive memory blocks.
void swap_mem(MemoryBlock* a, MemoryBlock* b) {
    b->start = a->start;
    b->end = b->start + b->size - 1;

    a->start = b->end + 1;
    a->end = a->start + a->size - 1;


    b->prev = a->prev;
    if (a->prev != NULL)
        a->prev->next = b;

    a->next = b->next;
    if (b->next != NULL)
        b->next->prev = a;

    a->prev = b;
    b->next = a;
}

// compact all holes at the start into one hole.
void compact_mem_holes(MemoryBlock* head) {
    while (head->next != NULL && head->next->name == NULL) {
        MemoryBlock* next = head->next;

        head->size += next->size;
        head->end = head->start + head->size - 1;

        head->next = next->next;
        if (next->next != NULL)
            next->next->prev = head;
        
        free(next);
    }
}

void compact_mem(MemoryBlock* head) {
    MemoryBlock* original_head = head;
    // While next is a hole:
    //      swap with current.
    while (head != NULL) {
        // If this is a hole.
        if (head->name == NULL) {
            MemoryBlock* current = head;
            // While previous is a process, not a hole.
            while (current->name == NULL && current->prev != NULL && current->prev->name != NULL) {
                swap_mem(current->prev, current);
            }
        }

        head = head->next;
    }

    // Merge all holes into one hole.
    compact_mem_holes(original_head);
}

int main(int argc, const char* argv[]) {
    // if (argc != 2) {
    //     printf("Invalid arguments.\nTerminating...\n");
    //     exit(-1);
    // }

    MemoryBlock head;
    // init_memory(&head, atoi(argv[1]));
    init_memory(&head, 1048576);


    print_stat(&head);

    printf("After allocation:\n");

    allocate_mem(&head, 51300, "P1", STRATEGY_FIRST_FIT);
    allocate_mem(&head, 89123, "P2", STRATEGY_FIRST_FIT);
    allocate_mem(&head, 49144, "P6", STRATEGY_FIRST_FIT);
    allocate_mem(&head, 81297, "P3", STRATEGY_FIRST_FIT);
    allocate_mem(&head, 777712, "P4", STRATEGY_FIRST_FIT);


    print_stat(&head);

    printf("After release:\n");

    release_mem(&head, "P6");
    // release_mem(&head, "P2");
    release_mem(&head, "P1");

    print_stat(&head);

    printf("After compaction:\n");
    compact_mem(&head);

    print_stat(&head);


    free_mem(&head);

    return 0;
}



