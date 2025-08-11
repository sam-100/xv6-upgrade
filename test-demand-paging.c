#include "user.h"
#include "util.h"

void main(int argc, char **argv) {
    // 1. Query initial number of virtual and physical pages to OS
    printf(STD_OUT, "Initially, \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);

    // 2. Allocate 5 new pages and query numvp() and numpp() again
    char *program_break = sbrk(0);
    char *new_page[5];
    for(int i=0; i<5; i++) {
        new_page[i] = (char*)(program_break + 7*i*PAGESIZE);            // select an address at an interval of every 7 page
        if(mmap(new_page[i], PAGESIZE) < 0)                             // allocate a page at that location (lazily)
            error("mmap failed.");
    }
    
    // 3. Print virtual and physical page count
    printf(STD_OUT, "After mmap, \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);
    if(fork() == 0) {
        printf(STD_OUT, "In children process:\n");
        printf(STD_OUT, "number of virtual pages = %d\n", numvp());
        printf(STD_OUT, "number of physical pages = %d\n", numpp());
        println(STD_OUT);
        exit();
    }
    wait();


    // 4. Now write to 2 of the allocated pages
    char *a;
    int *b;
    a = (char*)(new_page[2]+34);
    b = (int*)(new_page[4]+293);
    *a = 'm';
    *b = 69;

    // 4. Query number of virtual and physical pages again
    printf(STD_OUT, "After writing to new page 2 and 4, \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);

    // 5. Test copyuvm()
    if(fork() == 0) {
        printf(STD_OUT, "Inside child process: \n");
        printf(STD_OUT, "number of virtual pages = %d\n", numvp());
        printf(STD_OUT, "number of physical pages = %d\n", numpp());
        printf(STD_OUT, "a points to %c\n", *a);
        printf(STD_OUT, "b points to %d\n", *b);
        println(STD_OUT);
        exit();
    }
    wait();

    // 6. Unmapping 2 pages from virtual address space
    printf(STD_OUT, "Unmapping new page number 2 (addr = %x) and 3 (addr = %x)\n", new_page[2], new_page[3]);
    if(munmap(new_page[2], PAGESIZE) < 0)
        error("munmap failed.");
    if(munmap(new_page[3], PAGESIZE) < 0)
        error("munmap failed");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());

    exit();
}