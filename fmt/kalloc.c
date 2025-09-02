3400 // Physical memory allocator, intended to allocate
3401 // memory for user processes, kernel stacks, page table pages,
3402 // and pipe buffers. Allocates 4096-byte pages.
3403 
3404 #include "types.h"
3405 #include "defs.h"
3406 #include "param.h"
3407 #include "memlayout.h"
3408 #include "mmu.h"
3409 #include "spinlock.h"
3410 
3411 void freerange(void *vstart, void *vend);
3412 extern char end[]; // first address after kernel loaded from ELF file
3413                    // defined by the kernel linker script in kernel.ld
3414 
3415 struct run {
3416   struct run *next;
3417 };
3418 
3419 struct {
3420   struct spinlock lock;
3421   int use_lock;
3422   struct run *freelist;
3423 } kmem;
3424 
3425 // Initialization happens in two phases.
3426 // 1. main() calls kinit1() while still using entrypgdir to place just
3427 // the pages mapped by entrypgdir on free list.
3428 // 2. main() calls kinit2() with the rest of the physical pages
3429 // after installing a full page table that maps them on all cores.
3430 void
3431 kinit1(void *vstart, void *vend)
3432 {
3433   initlock(&kmem.lock, "kmem");
3434   kmem.use_lock = 0;
3435   freerange(vstart, vend);
3436 }
3437 
3438 void
3439 kinit2(void *vstart, void *vend)
3440 {
3441   freerange(vstart, vend);
3442   kmem.use_lock = 1;
3443 }
3444 
3445 
3446 
3447 
3448 
3449 
3450 void
3451 freerange(void *vstart, void *vend)
3452 {
3453   char *p;
3454   p = (char*)PGROUNDUP((uint)vstart);
3455   for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
3456     kfree(p);
3457 }
3458 
3459 // Free the page of physical memory pointed at by v,
3460 // which normally should have been returned by a
3461 // call to kalloc().  (The exception is when
3462 // initializing the allocator; see kinit above.)
3463 void
3464 kfree(char *v)
3465 {
3466   struct run *r;
3467 
3468   if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
3469     panic("kfree");
3470 
3471   // Fill with junk to catch dangling refs.
3472   memset(v, 1, PGSIZE);
3473 
3474   if(kmem.use_lock)
3475     acquire(&kmem.lock);
3476   r = (struct run*)v;
3477   r->next = kmem.freelist;
3478   kmem.freelist = r;
3479   if(kmem.use_lock)
3480     release(&kmem.lock);
3481 }
3482 
3483 // Allocate one 4096-byte page of physical memory.
3484 // Returns a pointer that the kernel can use.
3485 // Returns 0 if the memory cannot be allocated.
3486 char*
3487 kalloc(void)
3488 {
3489   struct run *r;
3490 
3491   if(kmem.use_lock)
3492     acquire(&kmem.lock);
3493   r = kmem.freelist;
3494   if(r)
3495     kmem.freelist = r->next;
3496   if(kmem.use_lock)
3497     release(&kmem.lock);
3498   return (char*)r;
3499 }
