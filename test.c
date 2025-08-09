#include "user.h"

void main(int argc, char **argv) {
    printf(STD_OUT, "Number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "Number of physical pages = %d\n", numpp());
    printf(STD_OUT, "Number of pages allocated for page table = %d\n", getptsize());
    printf(STD_OUT, "\n");
    
    sbrk(5*4096);   // grow current process's virtual pages by 5 more pages
    printf(STD_OUT, "After growing virtual memory by 5 pages: \n");
    printf(STD_OUT, "Number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "Number of physical pages = %d\n", numpp());
    printf(STD_OUT, "Number of pages allocated for page table = %d\n", getptsize());
    printf(STD_OUT, "\n");

    sbrk(-3*4096);   // shrink current process's virtual pages by 3 more pages
    printf(STD_OUT, "After shrinking virtual memory by 3 pages: \n");
    printf(STD_OUT, "Number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "Number of physical pages = %d\n", numpp());
    printf(STD_OUT, "Number of pages allocated for page table = %d\n", getptsize());
    printf(STD_OUT, "\n");
    
    exit();
}