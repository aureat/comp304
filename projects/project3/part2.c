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
#define PAGE_FRAMES 256
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
 * Frame Table
 *********************************************************************/

struct frameentry {
    int logical;
    int references;
    int reference_bit;
};

// Frame table is kept track of as a circular array
struct frameentry frames[PAGES];

// Page Replacement Policies
#define SECOND_CHANCE 0
#define LRU           1
#define FIFO          2

// Page replacement policy
int policy;

// Pointer for Second Chance policy
int clock_pointer = 0;

// Number of the next unallocated physical page in main memory
int fifo_pointer = 0;

// Finds a page frame to replace, based on the replacement policy
int replace_page_frame() {
    int page_to_evict = -1;

    /*
     * Second chance policy
     * Initially, all reference bits are set to 0.
     * When a page is referenced, its reference bit is set to 1.
     * When a page needs to be replaced, we start at the current frame index and
     * iterate through the frames until we find a page with a reference bit of 0.
     * If the reference bit is 1, we give it a second chance set it to 0 and continue iterating.
     * If the reference bit is 0, we evict that page.
     * If we reach the end of the frames, we start over at the beginning.
     */
    if (policy == SECOND_CHANCE) {
        while (frames[clock_pointer].reference_bit == 1) {
            frames[clock_pointer].reference_bit = 0;
            clock_pointer = (clock_pointer + 1) % PAGE_FRAMES;
        }
        page_to_evict = clock_pointer;
    }

    /*
     * Least recently used policy
     * Initially, all references are set to 0.
     * When a page is referenced, its reference is set to the current counter.
     * When a page needs to be replaced, we iterate through the frames and find the page with the lowest references.
     * If two pages have the same references, we evict the first one we find.
     */
    else if (policy == LRU) {
        page_to_evict = 0;
        int least_references = 0;
        for (int i = 0; i < PAGE_FRAMES; i++) {
            if (frames[i].references < least_references) {
                least_references = frames[i].references;
                page_to_evict = i;
            }
        }
    }

    /*
     * First in first out policy (for debugging purposes)
     * Implemented as a circular queue of free pages.
     */
    else if (policy == FIFO) {
        page_to_evict = fifo_pointer;
        fifo_pointer = (fifo_pointer + 1) % PAGE_FRAMES;
    }

    // Return the page to evict
    return page_to_evict;
}

/*********************************************************************
 * Stats
 *********************************************************************/

int total_addresses = 0;
int tlb_hits = 0;
int page_faults = 0;

// Usage info
void print_usage() {
    fprintf(stderr, "Usage ./part2 backingstore input -p policy\n");
    exit(0);
}

int main(int argc, const char *argv[]) {
    // Check usage
    if (argc != 5) {
        print_usage();
    }

    // Get backing from file
    const char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    // Get input file
    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Get replacement policy from command line
    if (strcmp(argv[3], "-p") != 0) {
        print_usage();
    }
    policy = atoi(argv[4]);

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

    // Initialize page frames
    for (i = 0; i < PAGES; i++) {
        frames[i].logical = -1;
        frames[i].references = 0;
        frames[i].reference_bit = 0;
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

        // printf("Accessing logical %u\n", logical_page);

        // look for the page in the TLB
        unsigned int physical_page = search_tlb(logical_page);

        // TLB hit
        if (physical_page != -1) {
            tlb_hits++;
        }

        // TLB miss
        else {
            // Look for the page in the page table
            physical_page = pagetable[logical_page];

            // Page fault
            if (physical_page == -1) {
                page_faults++;

                // If there is a free page frame, use it
                if (free_page < PAGE_FRAMES) {
                    physical_page = free_page;
                    free_page++;
                }

                // Otherwise, replace a page frame
                else {
                    physical_page = replace_page_frame();

                    // Prevent any subsequent lookup from pointing to the wrong page
                    pagetable[frames[physical_page].logical] = -1;
                }

                // Read page from backing file
                memcpy(main_memory + physical_page * PAGE_SIZE,
                       backing + logical_page * PAGE_SIZE, PAGE_SIZE);

                // Update frame attributes
                frames[physical_page].logical = logical_page;

                // Update page table
                pagetable[logical_page] = physical_page;
            }

            // Update TLB
            add_to_tlb(logical_page, physical_page);
        }

        // Update age and reference bit with each use
        // frames[physical_page].references = total_addresses;
        frames[physical_page].reference_bit = 1;

        // Update reference bit of TLB entry
        update_tlb(logical_page);

        // Read physical memory and print value
        unsigned int physical_address = (physical_page << OFFSET_BITS) | offset;
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];

        // printf("Virtual address: %d Physical address: %d Value: %d\n",
        //        logical_address, physical_address, value);
    }

    // Print out stats
    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
