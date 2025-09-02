2550 #include "types.h"
2551 #include "defs.h"
2552 #include "param.h"
2553 #include "memlayout.h"
2554 #include "mmu.h"
2555 #include "x86.h"
2556 #include "proc.h"
2557 #include "spinlock.h"
2558 #include "sh_mem.h"
2559 
2560 struct {
2561   struct spinlock lock;
2562   struct proc proc[NPROC];
2563 } ptable;
2564 
2565 static struct proc *initproc;
2566 
2567 int nextpid = 1;
2568 extern void forkret(void);
2569 extern void trapret(void);
2570 
2571 static void wakeup1(void *chan);
2572 
2573 void
2574 pinit(void)
2575 {
2576   initlock(&ptable.lock, "ptable");
2577 }
2578 
2579 // Must be called with interrupts disabled
2580 int
2581 cpuid() {
2582   return mycpu()-cpus;
2583 }
2584 
2585 // Must be called with interrupts disabled to avoid the caller being
2586 // rescheduled between reading lapicid and running through the loop.
2587 struct cpu*
2588 mycpu(void)
2589 {
2590   int apicid, i;
2591 
2592   if(readeflags()&FL_IF)
2593     panic("mycpu called with interrupts enabled\n");
2594 
2595   apicid = lapicid();
2596   // APIC IDs are not guaranteed to be contiguous. Maybe we should have
2597   // a reverse map, or reserve a register to store &cpus[i].
2598   for (i = 0; i < ncpu; ++i) {
2599     if (cpus[i].apicid == apicid)
2600       return &cpus[i];
2601   }
2602   panic("unknown apicid\n");
2603 }
2604 
2605 // Disable interrupts so that we are not rescheduled
2606 // while reading proc from the cpu structure
2607 struct proc*
2608 myproc(void) {
2609   struct cpu *c;
2610   struct proc *p;
2611   pushcli();
2612   c = mycpu();
2613   p = c->proc;
2614   popcli();
2615   return p;
2616 }
2617 
2618 static int get_smallest_pid() {
2619   int pids[NPROC];
2620   for(int i=0; i<NPROC; i++)
2621     pids[i] = 0;  // mark as free
2622 
2623   for(struct proc *p=ptable.proc; p < ptable.proc+NPROC; p++) {
2624     if(p->state != UNUSED) {
2625       pids[p->pid] = 1;
2626     }
2627   }
2628 
2629   int pid = -1;
2630   for(int i=0; i<NPROC; i++) {
2631     if(pids[i] == 0) {
2632       pid = i;
2633       break;
2634     }
2635   }
2636   return pid;
2637 
2638 }
2639 
2640 
2641 
2642 
2643 
2644 
2645 
2646 
2647 
2648 
2649 
2650 // Look in the process table for an UNUSED proc.
2651 // If found, change state to EMBRYO and initialize
2652 // state required to run in the kernel.
2653 // Otherwise return 0.
2654 static struct proc*
2655 allocproc(void)
2656 {
2657   struct proc *p;
2658   char *sp;
2659 
2660   acquire(&ptable.lock);
2661 
2662   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2663     if(p->state == UNUSED)
2664       goto found;
2665 
2666   release(&ptable.lock);
2667   return 0;
2668 
2669 found:
2670   p->state = EMBRYO;
2671   // p->pid = nextpid++;
2672   p->pid = get_smallest_pid();
2673 
2674   release(&ptable.lock);
2675 
2676   // Allocate kernel stack.
2677   if((p->kstack = kalloc()) == 0){
2678     p->state = UNUSED;
2679     return 0;
2680   }
2681   sp = p->kstack + KSTACKSIZE;
2682 
2683   // Leave room for trap frame.
2684   sp -= sizeof *p->tf;
2685   p->tf = (struct trapframe*)sp;
2686 
2687   // Set up new context to start executing at forkret,
2688   // which returns to trapret.
2689   sp -= 4;
2690   *(uint*)sp = (uint)trapret;
2691 
2692   sp -= sizeof *p->context;
2693   p->context = (struct context*)sp;
2694   memset(p->context, 0, sizeof *p->context);
2695   p->context->eip = (uint)forkret;
2696 
2697   p->shm_idx = -1;
2698   p->shm_va = 0;
2699 
2700   return p;
2701 }
2702 
2703 
2704 // Set up first user process.
2705 void
2706 userinit(void)
2707 {
2708   struct proc *p;
2709   extern char _binary_initcode_start[], _binary_initcode_size[];
2710 
2711   p = allocproc();
2712 
2713   initproc = p;
2714   if((p->pgdir = setupkvm()) == 0)
2715     panic("userinit: out of memory?");
2716   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2717   p->sz = PGSIZE;
2718   memset(p->tf, 0, sizeof(*p->tf));
2719   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2720   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2721   p->tf->es = p->tf->ds;
2722   p->tf->ss = p->tf->ds;
2723   p->tf->eflags = FL_IF;
2724   p->tf->esp = PGSIZE;
2725   p->tf->eip = 0;  // beginning of initcode.S
2726 
2727   safestrcpy(p->name, "initcode", sizeof(p->name));
2728   p->cwd = namei("/");
2729 
2730   // this assignment to p->state lets other cores
2731   // run this process. the acquire forces the above
2732   // writes to be visible, and the lock is also needed
2733   // because the assignment might not be atomic.
2734   acquire(&ptable.lock);
2735 
2736   p->state = RUNNABLE;
2737 
2738   release(&ptable.lock);
2739 }
2740 
2741 
2742 
2743 
2744 
2745 
2746 
2747 
2748 
2749 
2750 // Grow current process's memory by n bytes.
2751 // Return 0 on success, -1 on failure.
2752 int
2753 growproc(int n)
2754 {
2755   uint sz;
2756   struct proc *curproc = myproc();
2757 
2758   sz = curproc->sz;
2759   if(n > 0){
2760     if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
2761       return -1;
2762   } else if(n < 0){
2763     if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
2764       return -1;
2765   }
2766   curproc->sz = sz;
2767   switchuvm(curproc);
2768   return 0;
2769 }
2770 
2771 // Lazy version of growproc
2772 void *growproc_lazy(int n) {
2773   struct proc *curr_proc = myproc();
2774 
2775   int sz = curr_proc->sz;
2776   if(n > 0) {
2777     if((sz = allocuvm_lazy(curr_proc->pgdir, sz, sz + n)) == 0)
2778       return -1;
2779   } else if(n < 0){
2780     if((sz = deallocuvm(curr_proc->pgdir, sz, sz + n)) == 0)
2781       return -1;
2782   }
2783   curr_proc->sz = sz;
2784   switchuvm(curr_proc);
2785   return 0;
2786 }
2787 
2788 
2789 
2790 
2791 
2792 
2793 
2794 
2795 
2796 
2797 
2798 
2799 
2800 // Create a new process copying p as the parent.
2801 // Sets up stack to return as if from system call.
2802 // Caller must set state of returned proc to RUNNABLE.
2803 int
2804 fork(void)
2805 {
2806   int i, pid;
2807   struct proc *np;
2808   struct proc *curproc = myproc();
2809 
2810   // Allocate process.
2811   if((np = allocproc()) == 0){
2812     return -1;
2813   }
2814 
2815   // Copy process state from proc.
2816   if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
2817     kfree(np->kstack);
2818     np->kstack = 0;
2819     np->state = UNUSED;
2820     return -1;
2821   }
2822   np->sz = curproc->sz;
2823   np->parent = curproc;
2824   *np->tf = *curproc->tf;
2825 
2826   // Clear %eax so that fork returns 0 in the child.
2827   np->tf->eax = 0;
2828 
2829   for(i = 0; i < NOFILE; i++)
2830     if(curproc->ofile[i])
2831       np->ofile[i] = filedup(curproc->ofile[i]);
2832   np->cwd = idup(curproc->cwd);
2833 
2834   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
2835 
2836   pid = np->pid;
2837 
2838   acquire(&ptable.lock);
2839 
2840   np->state = RUNNABLE;
2841 
2842   release(&ptable.lock);
2843 
2844   return pid;
2845 }
2846 
2847 
2848 
2849 
2850 // Exit the current process.  Does not return.
2851 // An exited process remains in the zombie state
2852 // until its parent calls wait() to find out it exited.
2853 void
2854 exit(void)
2855 {
2856   struct proc *curproc = myproc();
2857   struct proc *p;
2858   int fd;
2859 
2860   if(curproc == initproc)
2861     panic("init exiting");
2862 
2863   // Close all open files.
2864   for(fd = 0; fd < NOFILE; fd++){
2865     if(curproc->ofile[fd]){
2866       fileclose(curproc->ofile[fd]);
2867       curproc->ofile[fd] = 0;
2868     }
2869   }
2870 
2871   begin_op();
2872   iput(curproc->cwd);
2873   end_op();
2874   curproc->cwd = 0;
2875 
2876   acquire(&ptable.lock);
2877 
2878   // Parent might be sleeping in wait().
2879   wakeup1(curproc->parent);
2880 
2881   // Pass abandoned children to init.
2882   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2883     if(p->parent == curproc){
2884       p->parent = initproc;
2885       if(p->state == ZOMBIE)
2886         wakeup1(initproc);
2887     }
2888   }
2889 
2890   // Jump into the scheduler, never to return.
2891   curproc->state = ZOMBIE;
2892   sched();
2893   panic("zombie exit");
2894 }
2895 
2896 
2897 
2898 
2899 
2900 // Wait for a child process to exit and return its pid.
2901 // Return -1 if this process has no children.
2902 int
2903 wait(void)
2904 {
2905   struct proc *p;
2906   int havekids, pid;
2907   struct proc *curproc = myproc();
2908 
2909   acquire(&ptable.lock);
2910   for(;;){
2911     // Scan through table looking for exited children.
2912     havekids = 0;
2913     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2914       if(p->parent != curproc)
2915         continue;
2916       havekids = 1;
2917       if(p->state == ZOMBIE){
2918         // Found one.
2919         pid = p->pid;
2920         kfree(p->kstack);
2921         p->kstack = 0;
2922         freevm(p->pgdir);
2923         p->pid = 0;
2924         p->parent = 0;
2925         p->name[0] = 0;
2926         p->killed = 0;
2927         p->state = UNUSED;
2928         release(&ptable.lock);
2929         return pid;
2930       }
2931     }
2932 
2933     // No point waiting if we don't have any children.
2934     if(!havekids || curproc->killed){
2935       release(&ptable.lock);
2936       return -1;
2937     }
2938 
2939     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2940     sleep(curproc, &ptable.lock);  //DOC: wait-sleep
2941   }
2942 }
2943 
2944 
2945 
2946 
2947 
2948 
2949 
2950 // Per-CPU process scheduler.
2951 // Each CPU calls scheduler() after setting itself up.
2952 // Scheduler never returns.  It loops, doing:
2953 //  - choose a process to run
2954 //  - swtch to start running that process
2955 //  - eventually that process transfers control
2956 //      via swtch back to the scheduler.
2957 void
2958 scheduler(void)
2959 {
2960   struct proc *p;
2961   struct cpu *c = mycpu();
2962   c->proc = 0;
2963 
2964   for(;;){
2965     // Enable interrupts on this processor.
2966     sti();
2967 
2968     // Loop over process table looking for process to run.
2969     acquire(&ptable.lock);
2970     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2971       if(p->state != RUNNABLE)
2972         continue;
2973 
2974       // Switch to chosen process.  It is the process's job
2975       // to release ptable.lock and then reacquire it
2976       // before jumping back to us.
2977       c->proc = p;
2978       switchuvm(p);
2979       p->state = RUNNING;
2980 
2981       swtch(&(c->scheduler), p->context);
2982       switchkvm();
2983 
2984       // Process is done running for now.
2985       // It should have changed its p->state before coming back.
2986       c->proc = 0;
2987     }
2988     release(&ptable.lock);
2989 
2990   }
2991 }
2992 
2993 
2994 
2995 
2996 
2997 
2998 
2999 
3000 // Enter scheduler.  Must hold only ptable.lock
3001 // and have changed proc->state. Saves and restores
3002 // intena because intena is a property of this
3003 // kernel thread, not this CPU. It should
3004 // be proc->intena and proc->ncli, but that would
3005 // break in the few places where a lock is held but
3006 // there's no process.
3007 void
3008 sched(void)
3009 {
3010   int intena;
3011   struct proc *p = myproc();
3012 
3013   if(!holding(&ptable.lock))
3014     panic("sched ptable.lock");
3015   if(mycpu()->ncli != 1)
3016     panic("sched locks");
3017   if(p->state == RUNNING)
3018     panic("sched running");
3019   if(readeflags()&FL_IF)
3020     panic("sched interruptible");
3021   intena = mycpu()->intena;
3022   swtch(&p->context, mycpu()->scheduler);
3023   mycpu()->intena = intena;
3024 }
3025 
3026 // Give up the CPU for one scheduling round.
3027 void
3028 yield(void)
3029 {
3030   acquire(&ptable.lock);  //DOC: yieldlock
3031   myproc()->state = RUNNABLE;
3032   sched();
3033   release(&ptable.lock);
3034 }
3035 
3036 
3037 
3038 
3039 
3040 
3041 
3042 
3043 
3044 
3045 
3046 
3047 
3048 
3049 
3050 // A fork child's very first scheduling by scheduler()
3051 // will swtch here.  "Return" to user space.
3052 void
3053 forkret(void)
3054 {
3055   static int first = 1;
3056   // Still holding ptable.lock from scheduler.
3057   release(&ptable.lock);
3058 
3059   if (first) {
3060     // Some initialization functions must be run in the context
3061     // of a regular process (e.g., they call sleep), and thus cannot
3062     // be run from main().
3063     first = 0;
3064     iinit(ROOTDEV);
3065     initlog(ROOTDEV);
3066   }
3067 
3068   // Return to "caller", actually trapret (see allocproc).
3069 }
3070 
3071 // Atomically release lock and sleep on chan.
3072 // Reacquires lock when awakened.
3073 void
3074 sleep(void *chan, struct spinlock *lk)
3075 {
3076   struct proc *p = myproc();
3077 
3078   if(p == 0)
3079     panic("sleep");
3080 
3081   if(lk == 0)
3082     panic("sleep without lk");
3083 
3084   // Must acquire ptable.lock in order to
3085   // change p->state and then call sched.
3086   // Once we hold ptable.lock, we can be
3087   // guaranteed that we won't miss any wakeup
3088   // (wakeup runs with ptable.lock locked),
3089   // so it's okay to release lk.
3090   if(lk != &ptable.lock){  //DOC: sleeplock0
3091     acquire(&ptable.lock);  //DOC: sleeplock1
3092     release(lk);
3093   }
3094   // Go to sleep.
3095   p->chan = chan;
3096   p->state = SLEEPING;
3097 
3098   sched();
3099 
3100   // Tidy up.
3101   p->chan = 0;
3102 
3103   // Reacquire original lock.
3104   if(lk != &ptable.lock){  //DOC: sleeplock2
3105     release(&ptable.lock);
3106     acquire(lk);
3107   }
3108 }
3109 
3110 
3111 
3112 
3113 
3114 
3115 
3116 
3117 
3118 
3119 
3120 
3121 
3122 
3123 
3124 
3125 
3126 
3127 
3128 
3129 
3130 
3131 
3132 
3133 
3134 
3135 
3136 
3137 
3138 
3139 
3140 
3141 
3142 
3143 
3144 
3145 
3146 
3147 
3148 
3149 
3150 // Wake up all processes sleeping on chan.
3151 // The ptable lock must be held.
3152 static void
3153 wakeup1(void *chan)
3154 {
3155   struct proc *p;
3156 
3157   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
3158     if(p->state == SLEEPING && p->chan == chan)
3159       p->state = RUNNABLE;
3160 }
3161 
3162 // Wake up all processes sleeping on chan.
3163 void
3164 wakeup(void *chan)
3165 {
3166   acquire(&ptable.lock);
3167   wakeup1(chan);
3168   release(&ptable.lock);
3169 }
3170 
3171 // Kill the process with the given pid.
3172 // Process won't exit until it returns
3173 // to user space (see trap in trap.c).
3174 int
3175 kill(int pid)
3176 {
3177   struct proc *p;
3178 
3179   acquire(&ptable.lock);
3180   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
3181     if(p->pid == pid){
3182       p->killed = 1;
3183       // Wake process from sleep if necessary.
3184       if(p->state == SLEEPING)
3185         p->state = RUNNABLE;
3186       release(&ptable.lock);
3187       return 0;
3188     }
3189   }
3190   release(&ptable.lock);
3191   return -1;
3192 }
3193 
3194 
3195 
3196 
3197 
3198 
3199 
3200 // Print a process listing to console.  For debugging.
3201 // Runs when user types ^P on console.
3202 // No lock to avoid wedging a stuck machine further.
3203 void
3204 procdump(void)
3205 {
3206   static char *states[] = {
3207   [UNUSED]    "unused",
3208   [EMBRYO]    "embryo",
3209   [SLEEPING]  "sleep ",
3210   [RUNNABLE]  "runble",
3211   [RUNNING]   "run   ",
3212   [ZOMBIE]    "zombie"
3213   };
3214   int i;
3215   struct proc *p;
3216   char *state;
3217   uint pc[10];
3218 
3219   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
3220     if(p->state == UNUSED)
3221       continue;
3222     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
3223       state = states[p->state];
3224     else
3225       state = "???";
3226     cprintf("%d %s %s", p->pid, state, p->name);
3227     if(p->state == SLEEPING){
3228       getcallerpcs((uint*)p->context->ebp+2, pc);
3229       for(i=0; i<10 && pc[i] != 0; i++)
3230         cprintf(" %p", pc[i]);
3231     }
3232     cprintf("\n");
3233   }
3234 }
3235 
3236 
3237 void sys_greet(void) {
3238   char *name;
3239   argstr(0, &name);
3240 
3241   cprintf("Hello %s\n", name);
3242 }
3243 
3244 
3245 
3246 
3247 
3248 
3249 
3250 static char *map_last_page(uint pa, uint perm) {
3251   struct proc *p = myproc();
3252   pde_t *pgdir = p->pgdir;
3253   for(int i = (KERNBASE >> PDXSHIFT); i >= 0; i--) {
3254     pde_t *pde = &pgdir[i];
3255     if(!(*pde & PTE_P)) {
3256       char *va = kalloc();
3257       memset(va, 0, PGSIZE);
3258       *pde = (V2P(va) | PTE_P | PTE_W);    // allocate a new page table
3259     }
3260 
3261     pte_t *pgtable = P2V(PTE_ADDR(*pde));
3262     for(int j=NPTENTRIES-1; j >= 0; j--) {
3263       pte_t *pte = &pgtable[j];
3264       if(*pte & PTE_P)
3265         continue;
3266 
3267       *pte = (pa | PTE_P | perm);         // map the page to page table entry
3268       return (char*)PGADDR(i, j, 0);
3269     }
3270   }
3271   return 0;
3272 }
3273 
3274 uint sys_shm_open(void) {
3275   struct proc *p = myproc();
3276   if(p->shm_va != 0)
3277     return p->shm_va;
3278 
3279   // 1. Map a new virtual page and allocate a physical page to it
3280   char *va;
3281   va = kalloc();
3282   if(va == 0) {
3283     cprintf("sys_shm_open: kalloc failed");
3284     return 0;
3285   }
3286   uint pa = (uint)V2P(va);
3287 
3288   char *uva;
3289   uva = map_last_page(pa, PTE_U | PTE_W);
3290   if(uva == 0) {
3291     cprintf("sys_shm_open: map_last_page failed.");
3292     return 0;
3293   }
3294 
3295   int idx = shm_add(pa);
3296   if(idx < 0) {
3297     cprintf("sys_shm_open: shm_add failed.\n");
3298     return 0;
3299   }
3300   p->shm_va = uva;
3301   p->shm_idx = idx;
3302 
3303   return uva;
3304 }
3305 
3306 uint sys_shm_get(void) {
3307   struct proc *p = myproc();
3308   return p->shm_va;
3309 }
3310 
3311 static pte_t *walkpgdir(pde_t *pgdir, char *uva) {
3312   pde_t *pde = &pgdir[PDX(uva)];
3313   if(!(*pde & PTE_P))   return 0;
3314   pte_t *pgtable = P2V(PTE_ADDR(*pde));
3315   pte_t *pte = &pgtable[PTX(uva)];
3316   return pte;
3317   return 0;
3318 }
3319 
3320 int sys_shm_close(void) {
3321   struct proc *p = myproc();
3322   if(p->shm_va == 0) {
3323     cprintf("sys_shm_close: attempted to close non-existing shared memory.");
3324     return -1;
3325   }
3326 
3327   int idx = p->shm_idx;
3328   pte_t *pte = walkpgdir(p->pgdir, p->shm_va);
3329   uint pa = PTE_ADDR(*pte);
3330   // uint pa = getpa(p->pgdir, p->shm_va);
3331   char *uva = p->shm_va;
3332 
3333   if(shm_remove(pa) < 0) {
3334     cprintf("sys_shm_close: shm_remove failed.");
3335     return -1;
3336   }
3337   *pte = 0;   // unmap the virtual page in pte
3338   p->shm_idx = -1;
3339   p->shm_va = 0;
3340 }
3341 
3342 
3343 
3344 
3345 
3346 
3347 
3348 
3349 
