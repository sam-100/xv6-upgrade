#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 64

int
main(void)
{
  char *shm = shm_open();
  if (shm == 0) {
    printf(1, "shm_open failed\n");
    exit();
  }

  printf(1, "Parent: shared memory address = %p\n", shm);

  int pid1 = fork();
  if (pid1 == 0) {
    // Child 1
    char *s = shm_get();
    if (s == 0) {
      printf(1, "Child1: shm_get failed\n");
      exit();
    }
    printf(1, "Child1: got shared memory at %p\n", s);
    strcpy(s, "Hello from child1");
    sleep(50); // give time for child2 to read/write
    printf(1, "Child1 sees in shared mem: %s\n", s);
    exit();
  }

  int pid2 = fork();
  if (pid2 == 0) {
    // Child 2
    char *s = shm_get();
    if (s == 0) {
      printf(1, "Child2: shm_get failed\n");
      exit();
    }
    printf(1, "Child2: got shared memory at %p\n", s);
    sleep(20); // wait for child1 to write
    printf(1, "Child2 sees in shared mem: %s\n", s);
    strcpy(s, "Hello from child2");
    printf(1, "Child2 wrote new message\n");
    exit();
  }

  // Parent waits for both
  wait();
  wait();

  printf(1, "Parent sees in shared mem: %s\n", shm);

  if (shm_close() == 0)
    printf(1, "Parent: closed shared memory\n");
  else
    printf(1, "Parent: failed to close shared memory\n");

  exit();
}
