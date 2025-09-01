#ifndef SH_MEM_H
#define SH_MEM_H

#include "types.h"
#include "param.h"
#include "spinlock.h"

typedef struct {
  char *pa;
  int ref_cnt;
} shared_memory_t;

void shm_init();  // initialize the shared_memory global data structure
int shm_add(const char *pa);  // add a new reference to shared page entry in the physical address in shared_memory data structure
int shm_remove(const char *pa); // remove a reference to shared page entry in the physical address in shared_memory data structure

#endif