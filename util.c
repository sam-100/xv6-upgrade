#include "util.h"
#include "user.h"

void error(const char *msg) {
    printf(STD_ERR, "Error: %s\n", msg);
    exit();
}

void println(int fd) {
    printf(fd, "\n");
}