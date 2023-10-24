## COMP 304: Operating Systems

### Assignment 1
Implemented basic CLI programs that use forks, pipes, and shared memory.

**See:** [Code](assignments/assignment1), [Project Description](assignment/assignment1/assignment_description.pdf)

### Project 1
Implemented a fish-like OS shell called **Mishell**.
- Supports basic builtin commands like `help`, `cd`, `exit`, as well as `ls` as a built-in.
- Supports background processes with `&`
- Supports IO redirects: `>`, `>>`, `<` 
- Supports the cd history `cdh` command just like in `fish`
- Supports the `cloc` command that recursively counts lines of code for all source code files in a given directory
- Supports a custom command called `michelle`, that "boots up" a hallucinated shell straight from ChatGPT
- Supports the `psvis` command that uses a kernel module to visualize the subprocess tree in a human-friendly graph form given a PID

**See:** [Code](projects/project1), [Project Description](projects/project1/project_description.pdf)

### Project 2
Implemented an electronic voting simulator using POSIX threads. The simulation supports multiple polling stations, each equipped with its own blocking priority queue. Voters may have priority based on their social group and polling stations may fail with a certain priority.

**See:** [Code](projects/project2), [Project Description](projects/project2/project_description.pdf)

### Project 3
Implemented a simulated virtual memory manager with a physical memory of size 256 and a virtual memory of size 1024 pages. The VM uses a TLB with a Second Chance replacement policy, as well as a page table with a LRU page-replacement policy.

**See:** [Code](projects/project3), [Project Description](projects/project3/project_description.pdf)

### Resources
- [Textbook](textbook.pdf)
- [Past and Similar Exams](exams)
- [Lecture Notes](notes)
- [Lecture Handouts](handouts)

