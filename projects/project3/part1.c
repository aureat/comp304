#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16

#define OFFSET_BITS 10
#define OFFSET_MASK ((1 << OFFSET_BITS) - 1)

#define PAGES       1024
#define PAGE_SIZE   1024
#define MEMORY_SIZE PAGES *PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

/*********************************************************************
 * TLB
 *********************************************************************/

struct tlbentry {
    unsigned int logical;
    unsigned int physical;
    unsigned char reference_bit;
};

// TLB is kept track of as a circular array,
int tlbindex = 0;
struct tlbentry tlb[TLB_SIZE];

// Returns the physical address from TLB or -1 if not present.
unsigned int search_tlb(unsigned int logical_page) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].logical == logical_page) {
            return tlb[i].physical;
        }
    }
    return -1;
}

// Updates the reference bit of the TLB entry for the specified page
void update_tlb(unsigned int logical_page) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].logical == logical_page) {
            tlb[i].reference_bit = 1;
        }
    }
}

// Adds the specified mapping to the TLB,
// TLB replacement using Second Chance (Clock).
void add_to_tlb(unsigned int logical, unsigned int physical) {
    // iterate until we find an empty spot or a reference bit is 0
    while (tlb[tlbindex % TLB_SIZE].reference_bit == 1) {
        tlb[tlbindex % TLB_SIZE].reference_bit = 0;
        tlbindex++;
    }

    // insert into TLB
    tlb[tlbindex % TLB_SIZE].logical = logical;
    tlb[tlbindex % TLB_SIZE].physical = physical;
    tlb[tlbindex % TLB_SIZE].reference_bit = 1;
    tlbindex++;
}

/*********************************************************************
 * Backing Memory
 **********************************************************************/

// Physical memory
signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

/*********************************************************************
 * Page Table
 *********************************************************************/

// pagetable[logical_page] is the physical page number for logical page.
// Value is -1 if that logical page isn't yet in the table.
unsigned int pagetable[PAGES];
int free_page = 0;

/*********************************************************************
 * Stats
 *********************************************************************/

int total_addresses = 0;
int tlb_hits = 0;
int page_faults = 0;

// Usage info
void print_usage() {
    fprintf(stderr, "Usage: ./part1 backingstore input\n");
    exit(0);
}

int main(int argc, const char *argv[]) {
    // Check usage
    if (argc != 3) {
        print_usage();
    }

    // Get backing from file
    const char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    // Get input file
    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++) {
        pagetable[i] = -1;
    }

    // Initialize TLB
    for (i = 0; i < TLB_SIZE; i++) {
        tlb[i].logical = -1;
        tlb[i].physical = -1;
        tlb[i].reference_bit = 0;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        // increment total addresses
        total_addresses++;

        // Calculate the page offset and logical page number from logical_address
        int logical_address = atoi(buffer);
        unsigned int offset = logical_address & OFFSET_MASK;
        unsigned int logical_page = (logical_address >> OFFSET_BITS) &
                                    OFFSET_MASK;

        printf("Accessing logical %u\n", logical_page);

        // look for the page in the TLB
        unsigned int physical_page = search_tlb(logical_page);

        // TLB hit
        if (physical_page != (unsigned int)-1) {
            tlb_hits++;
        }

        // TLB miss
        else {
            // Look for the page in the page table
            physical_page = pagetable[logical_page];

            // Page fault
            if (physical_page == -1) {
                page_faults++;

                // Assign next free page
                physical_page = free_page;

                // Copy page from backing file into physical memory
                memcpy(main_memory + physical_page * PAGE_SIZE,
                       backing + logical_page * PAGE_SIZE, PAGE_SIZE);

                // Update page table
                pagetable[logical_page] = physical_page;

                // find the next free page (FIFO replacement)
                free_page = (free_page + 1) % PAGES;
            }

            // Update TLB
            add_to_tlb(logical_page, physical_page);
        }

        // Update reference bit of TLB entry
        update_tlb(logical_page);

        // Read physical memory and print value
        unsigned int physical_address = (physical_page << OFFSET_BITS) | offset;
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];

        printf("Virtual address: %d Physical address: %d Value: %d\n",
               logical_address, physical_address, value);
    }

    // Print out stats
    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
