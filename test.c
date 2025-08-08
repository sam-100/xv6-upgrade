#include "user.h"

void main(int argc, char **argv) {

    int f = fork();
    if(f == 0) {
        sleep(10);
        printf(1, "child process %d: exiting\n", getpid());
        exit();
    } else if(f > 0) {
        int cpid = wait();
        printf(1, "parent process %d: child %d exited.\n", getpid(), f);
        exit();
    } else {
        printf(1, "Error: fork failed\n");
        exit();
    }
    int child_pid = wait();
    return;
}