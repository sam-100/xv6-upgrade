#include "user.h"
#include "util.h"

void main(int argc, char **argv) {
    // 1. Query initial number of virtual and physical pages to OS
    printf(STD_OUT, "Initially, \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);

    // 2. Allocate 5 new pages and query numvp() and numpp() again
    char *start = (char*)mmap(5*PAGESIZE);
    if(start == 0)
        error("mmap failed.");
    printf(STD_OUT, "After mmap(5), \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);


    // 3. Now write to 2 of the allocated pages
    char *pg2, *pg4;
    pg2 = start + 345;
    pg4 = start + 2*PAGESIZE + 345;
    
    printf(STD_OUT, "Accessing data at {page no. %d, addr 0x%x}, and {page no. %d, addr 0x%x}\n", 
        (uint)pg2/PAGESIZE, 
        pg2, 
        (uint)pg4/PAGESIZE, 
        pg4
    );
    *pg2 = 's';
    *pg4 = 'd';

    // 4. Query number of virtual and physical pages again
    printf(STD_OUT, "After writing to new page 3 and 5, \n");
    printf(STD_OUT, "number of virtual pages = %d\n", numvp());
    printf(STD_OUT, "number of physical pages = %d\n", numpp());
    println(STD_OUT);

    exit();
}