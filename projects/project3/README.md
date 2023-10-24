[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/EMsOClKs)

# COMP304: Project 3

*Project completed individually by Altun Hasanli.*

### Part 1

- TLB is implemented as a circular buffer with FIFO replacement.
- Since virtual memory is the same size as the physical memory, no special replacement policy is implemented.

### Part 2

- If page faults are less than the page frames, we use one of the free pages.
- Otherwise, for `-p 0`, we use **Second Chance** replacement policy:
    - Implemented as a FIFO circular buffer, `struct pageframe frames[PAGE_FRAMES]` stores the `reference_bit` attribute of the page frame to execute the replacement policy.
    - Initially all reference bits are 0.
    - When a page is referenced, its reference bit is set to 1.
    - When a page needs to be replaced, we start at the current frame index and iterate through the frames until we find a page with a reference bit of 0.
        - If the reference bit is 1, we give it a second chance, set it to 0 and continue iterating.
        - If the reference bit is 0, we evict the page.
- for `-p 1`, we use **LRU** replacement policy:
    - Implemented as a FIFO circular buffer, `struct pageframe frames[PAGE_FRAMES]` stores the `age` attribute of the page frame to execute the replacement policy.
    - Initially all ages are set to 0.
    - When a page is referenced, its age is set to the current `total_addresses` counter.
    - When a page needs to be replaced, we iterate through the pages and find the page with the lowest age.
    - If two pages have the same age, we evict the first one we find.
