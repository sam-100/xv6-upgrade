#include "user.h"
#include "util.h"

void thread_func(void *arg) {
    int id = *(int *)arg;
    printf(1, "Hello from thread %d\n", id);
    println(1);
    thread_exit();
}

int main() {
    int tid1, tid2;
    int arg1 = 1, arg2 = 2;

    tid1 = thread_create(thread_func, &arg1);
    assert(tid1 != -1, "Error: failed to create new thread.\n");
    tid2 = thread_create(thread_func, &arg2);
    assert(tid2 != -1, "Error: failed to create new thread.\n");

    assert(thread_join(tid1) == 0, "Failed to join thread");
    assert(thread_join(tid2) == 0, "Failed to join thread");

    printf(1, "All threads finished\n");
    println(1);

    exit();
}
