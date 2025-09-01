#include "sh_mem.h"
#include "defs.h"

shared_memory_t shared_memory[NPROC];
struct spinlock shm_lock;

void shm_init() {
  for(int i=0; i<NPROC; i++) {
    shared_memory[i].ref_cnt = 0;
    shared_memory[i].pa = 0;
    initlock(&shm_lock, "shm");
  }
}

int shm_add(const char *pa) {
  acquire(&shm_lock);

  // 1. First search if the page already exists
  for(int i=0; i<NPROC; i++) {
    if(shared_memory[i].pa == pa) {
      shared_memory[i].ref_cnt++;
      release(&shm_lock);
      return i;
    }
  }

  // 2. If it doesn't, then allocate a slot for the page, setup and return
  for(int i=0; i<NPROC; i++) {
    if(shared_memory[i].ref_cnt == 0) {
      shared_memory[i].pa = pa;
      shared_memory[i].ref_cnt = 1;
      release(&shm_lock);
      return i;
    }
  }
  release(&shm_lock);
  panic("shm_add: no slot available for shared page!");
  return -1;
}

int shm_remove(const char *pa) {
  acquire(&shm_lock);
  for(int i=0; i<NPROC; i++) {
    if(shared_memory[i].pa == pa) {
      shared_memory[i].ref_cnt--;
      if(shared_memory[i].ref_cnt == 0)
        shared_memory[i].pa = 0;
      release(&shm_lock);
      return 0;
    }
  }
  release(&shm_lock);
  panic("shm_remove: shared page not found.");
  return -1;
}