#include "user.h"
#include "util.h"

char *str = "Hello World!", *str2 = "Byeee";

void copy_str(char *dst, const char *src);

int main() {
    
    // 1. Open a shared memory page
    char *shared = shm_open();
    assert(shared != 0, "failed to open shared page.");
    printf(STD_OUT, "shared page address = %x\n", shared);

    // 2. Write on shared memory
    copy_str(shared, str);
    
    // 3. Read and print shared memory
    printf(STD_OUT, "Written to shared page: %s\n", shared);


    int f = fork();
    assert(f >= 0, "fork failed.");

    // 4. In child process: 
    if(f == 0) {            // Child process

        shared = shm_get();     // Get shared memory
        assert(shared != 0, "get_shm failed in child process.");

        // Read shared memory 
        printf(STD_OUT, "In child process: %s\n", shared);

        // Re-write shared memory
        copy_str(shared, str2);

        // Read shared memory
        printf(STD_OUT, "Child: shared memory = %s\n", shared);
        printf(STD_OUT, "Child dies\n\n");
        exit();
    }

    // 5. Wait for child to exit, and then read shared memory
    assert(wait() != -1, "Parent process says it has no children.");
    printf(STD_OUT, "In parent process, shared area: %s\n", shared);


    f = fork();
    if(f == 0) {
        char *shared = shm_get();
        copy_str(shared, "What's up Mumbai!");
        exit();
    } 
    
    wait();
    printf(STD_OUT, "Again, shared memory content: %s\n", shared);

    // 6. Close shared memory
    assert(shm_close() == 0, "shm_close() failed.");
    printf(STD_OUT, "Shared page closed.\n");


    f = fork();
    assert(f != -1, "fork failed.");
    if(f == 0) {
        assert(shm_get() == 0, "Shared page still open in child process.");
        exit();
    }
    
    printf(STD_OUT, "Test done!\n\n");
    exit();
}


void copy_str(char *dst, const char *src) {
    while(*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
}