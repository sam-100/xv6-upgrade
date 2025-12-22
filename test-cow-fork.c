#include "user.h"
#include "util.h"


void main(int argc, char **argv) {
    
    printf(STD_OUT, "Welcome to the copy-on-write fork testing program.\n");

    char *str = malloc(1024);
    strcpy(str, "Hello World!");

    uint va, pa;
    va = str;
    pa = va_to_pa(va);
    printf(STD_OUT, "Start: va = %p, pa = %p\n", va, pa);

    int f = fork();
    if(f < 0) {
        printf(STD_ERR, "Error: fork failed\n");
        exit();
    }


    if(f == 0) {
        uint child_va, child_pa;
        child_va = str;
        child_pa = va_to_pa(child_va);
        
        if(child_pa != pa) {
            printf(STD_ERR, "Error: physical address is different in children and parent processes.\n");
            exit();
        }
        printf(STD_OUT, "Child: physical address is same as parent.\n");

        strcpy(str, "Hola World");
        printf(STD_OUT, "Child: overwritten str\n");


        child_pa = va_to_pa(child_va);
        if(child_pa == pa) {
            printf(STD_ERR, "Error: child and parent physical address are still same!\n");
            exit();
        }
        printf(STD_OUT, "Child: Child and parent physical address are different.\n");

        printf(STD_OUT, "Child process exitting.\n");
        exit();
    }
    wait();


    if(strcmp(str, "Hello World!\n") != 0) {
        printf(STD_ERR, "Error: parent string changed.\n");
        exit();
    }


    printf(STD_OUT, "Test successful.\n");
    exit();

} 
