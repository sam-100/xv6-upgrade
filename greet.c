#include "user.h"

void main(int argc, char **argv) {
    if(argc == 1) {
        printf(1, "Usage: %s <name> [other names ...]\n", argv[0]);
        exit();
    }

    for(int i=1; i<argc; i++)
        greet(argv[i]);
    exit();
}