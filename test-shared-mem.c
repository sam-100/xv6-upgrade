#include "user.h"
#include "util.h"

char *str = "Hello World!", *str2 = "Byeee";


int main() {
    
    char *shared = shm_open();
    assert(shared != 0, "failed to open shared page.");

    int i = 0;
    for(; str[i] != '\0'; i++)
        shared[i] = str[i];
    shared[i] = '\0';
    
    printf(STD_OUT, "Written to shared page: %s\n", shared);

    int f = fork();
    assert(f >= 0, "fork failed.");

    if(f == 0) {
        shared = get_shm();
        assert(shared != 0, "get_shm failed in child process.");

        printf(STD_OUT, "In child process: %s\n", shared);
        for(int i=0; i<5; i++)
            shared[i] = str2[i];
        printf(STD_OUT, "String changed in child process.\n");

        printf(STD_OUT, "Child: shared memory = %s\n", shared);
        exit();
    } else {
        assert(wait() != -1, "Parent process says it has no children.");
        printf(STD_OUT, "In parent process, shared area: %s\n", shared);
    }

    exit();
}
