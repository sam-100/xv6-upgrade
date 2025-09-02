#include "sh_mem.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"

shared_memory_t shared_memory[N_SHARED_PAGES];
struct spinlock shm_lock;

void shm_init() {
  for(int i=0; i<N_SHARED_PAGES; i++) {
    shared_memory[i].ref_cnt = 0;
    shared_memory[i].pa = 0;
  }
  initlock(&shm_lock, "sh_mem");
}

int shm_add(uint pa) {
  acquire(&shm_lock);

  // 1. First search if the page already exists
  int free_idx = -1;
  for(int i=0; i<N_SHARED_PAGES; i++) {
    if(shared_memory[i].pa == pa) {
      shared_memory[i].ref_cnt++;
      release(&shm_lock);
      return i;
    }
    if(free_idx == -1 && shared_memory[i].ref_cnt == 0)
      free_idx = i;
  }

  // 2. If it doesn't, then allocate a slot for the page, setup and return
  if(free_idx == -1) {
    release(&shm_lock);
    cprintf("No more shared pages left.");
    return -1;
  }

  shared_memory[free_idx].pa = pa;
  shared_memory[free_idx].ref_cnt = 1;
  release(&shm_lock);
  return free_idx;
}

int shm_remove(uint pa) {
  acquire(&shm_lock);
  for(int i=0; i<N_SHARED_PAGES; i++) {
    if(shared_memory[i].pa != pa || shared_memory[i].ref_cnt == 0)
      continue;
    
    // Found the entry
    shared_memory[i].ref_cnt--;
    if(shared_memory[i].ref_cnt == 0) {
      shared_memory[i].pa = 0;
      kfree(P2V(pa));
    }
    release(&shm_lock);
    return 0;
  }

  release(&shm_lock);
  cprintf("shm_remove: shared page not found.");
  return -1;
}