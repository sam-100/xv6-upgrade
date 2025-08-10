#include "user.h"
#include "util.h"


void main(int argc, char **argv) {
    // 1. Get initial number of vp, pp, and free pages in system
    printf(STD_OUT, "Initially: \n");
    printf(STD_OUT, "vp = %d, pp = %d, total free pages = %d\n", numvp(), numpp(), getNumFreePages());

    // 2. allocate 5 pages from memory
    char *program_break = getProgramBreak();
    
    if(mmap(program_break, 5*PAGESIZE) < 0)
        error("mmap failed.\n");

    // 3. Write to the 5 allocated pages something
    for(int i=0; i<5; i++) {
        
    }
}