#ifndef SH_MEM_H
#define SH_MEM_H

#include "types.h"
#include "param.h"
#define N_SHARED_PAGES 64

typedef struct {
  uint pa;
  int ref_cnt;
} shared_memory_t;

void shm_init();  // initialize the shared_memory global data structure
int shm_add(uint pa);  // add a new reference to shared page entry in the physical address in shared_memory data structure
int shm_remove(uint pa); // remove a reference to shared page entry in the physical address in shared_memory data structure

#endif
