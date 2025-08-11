2450 // Per-CPU state
2451 struct cpu {
2452   uchar apicid;                // Local APIC ID
2453   struct context *scheduler;   // swtch() here to enter scheduler
2454   struct taskstate ts;         // Used by x86 to find stack for interrupt
2455   struct segdesc gdt[NSEGS];   // x86 global descriptor table
2456   volatile uint started;       // Has the CPU started?
2457   int ncli;                    // Depth of pushcli nesting.
2458   int intena;                  // Were interrupts enabled before pushcli?
2459   struct proc *proc;           // The process running on this cpu or null
2460 };
2461 
2462 extern struct cpu cpus[NCPU];
2463 extern int ncpu;
2464 
2465 
2466 // Saved registers for kernel context switches.
2467 // Don't need to save all the segment registers (%cs, etc),
2468 // because they are constant across kernel contexts.
2469 // Don't need to save %eax, %ecx, %edx, because the
2470 // x86 convention is that the caller has saved them.
2471 // Contexts are stored at the bottom of the stack they
2472 // describe; the stack pointer is the address of the context.
2473 // The layout of the context matches the layout of the stack in swtch.S
2474 // at the "Switch stacks" comment. Switch doesn't save eip explicitly,
2475 // but it is on the stack and allocproc() manipulates it.
2476 struct context {
2477   uint edi;
2478   uint esi;
2479   uint ebx;
2480   uint ebp;
2481   uint eip;
2482 };
2483 
2484 enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
2485 
2486 // Per-process state
2487 struct proc {
2488   uint sz;                     // Size of process memory (bytes)
2489   pde_t* pgdir;                // Page table
2490   char *kstack;                // Bottom of kernel stack for this process
2491   enum procstate state;        // Process state
2492   int pid;                     // Process ID
2493   struct proc *parent;         // Parent process
2494   struct trapframe *tf;        // Trap frame for current syscall
2495   struct context *context;     // swtch() here to run process
2496   void *chan;                  // If non-zero, sleeping on chan
2497   int killed;                  // If non-zero, have been killed
2498   struct file *ofile[NOFILE];  // Open files
2499   struct inode *cwd;           // Current directory
2500   char name[16];               // Process name (debugging)
2501 };
2502 
2503 // Process memory is laid out contiguously, low addresses first:
2504 //   text
2505 //   original data and bss
2506 //   fixed-size stack
2507 //   expandable heap
2508 
2509 
2510 
2511 
2512 
2513 
2514 
2515 
2516 
2517 
2518 
2519 
2520 
2521 
2522 
2523 
2524 
2525 
2526 
2527 
2528 
2529 
2530 
2531 
2532 
2533 
2534 
2535 
2536 
2537 
2538 
2539 
2540 
2541 
2542 
2543 
2544 
2545 
2546 
2547 
2548 
2549 
