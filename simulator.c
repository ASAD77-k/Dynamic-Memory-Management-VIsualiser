#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1024
#define PAGE_SIZE 64
#define NUM_PAGES (MEMORY_SIZE / PAGE_SIZE)

typedef struct {
    int page_id;
    int is_allocated;
    int process_id;
} Page;

Page physical_memory[NUM_PAGES];

void initialize_memory() {
    for (int i = 0; i < NUM_PAGES; i++) {
        physical_memory[i].page_id = i;
        physical_memory[i].is_allocated = 0;
        physical_memory[i].process_id = -1;
    }
}

void allocate_memory(int process_id, int num_pages_needed) {
    int pages_allocated = 0;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (!physical_memory[i].is_allocated) {
            physical_memory[i].is_allocated = 1;
            physical_memory[i].process_id = process_id;
            pages_allocated++;
            printf("Allocated Page %d to Process %d\n", i, process_id);
            if (pages_allocated == num_pages_needed) {
                printf("Successfully allocated %d pages to Process %d\n", num_pages_needed, process_id);
                return;
            }
        }
    }
    printf("Not enough contiguous or non-contiguous memory for Process %d\n", process_id);
}

void deallocate_memory(int process_id) {
    int pages_freed = 0;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (physical_memory[i].is_allocated && physical_memory[i].process_id == process_id) {
            physical_memory[i].is_allocated = 0;
            physical_memory[i].process_id = -1;
            pages_freed++;
            printf("Deallocated Page %d from Process %d\n", i, process_id);
        }
    }
    if (pages_freed > 0) {
        printf("Successfully freed %d pages from Process %d\n", pages_freed, process_id);
    } else {
        printf("Process %d not found in memory\n", process_id);
    }
}

void print_memory_map() {
    printf("\n--- Physical Memory Map ---\n");
    for (int i = 0; i < NUM_PAGES; i++) {
        if (physical_memory[i].is_allocated) {
            printf("[Page %2d] : Process %d\n", i, physical_memory[i].process_id);
        } else {
            printf("[Page %2d] : Free\n", i);
        }
    }
    printf("---------------------------\n\n");
}

int main() {
    printf("Dynamic Memory Management Simulator\n");
    initialize_memory();
    print_memory_map();
    
    printf("Simulating Allocations...\n");
    allocate_memory(1, 3);
    allocate_memory(2, 5);
    print_memory_map();
    
    printf("Simulating Deallocations...\n");
    deallocate_memory(1);
    print_memory_map();
    
    printf("Simulating Virtual Memory / Paging...\n");
    allocate_memory(3, 8);
    print_memory_map();
    
    return 0;
}
