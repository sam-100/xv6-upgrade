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

    tid1 = clone(thread_func, &arg1);
    tid2 = clone(thread_func, &arg2);

    thread_join(tid1);
    thread_join(tid2);

    printf(1, "All threads finished\n");
    println(1);

    exit();
}
