3500 // x86 trap and interrupt constants.
3501 
3502 // Processor-defined:
3503 #define T_DIVIDE         0      // divide error
3504 #define T_DEBUG          1      // debug exception
3505 #define T_NMI            2      // non-maskable interrupt
3506 #define T_BRKPT          3      // breakpoint
3507 #define T_OFLOW          4      // overflow
3508 #define T_BOUND          5      // bounds check
3509 #define T_ILLOP          6      // illegal opcode
3510 #define T_DEVICE         7      // device not available
3511 #define T_DBLFLT         8      // double fault
3512 // #define T_COPROC      9      // reserved (not used since 486)
3513 #define T_TSS           10      // invalid task switch segment
3514 #define T_SEGNP         11      // segment not present
3515 #define T_STACK         12      // stack exception
3516 #define T_GPFLT         13      // general protection fault
3517 #define T_PGFLT         14      // page fault
3518 // #define T_RES        15      // reserved
3519 #define T_FPERR         16      // floating point error
3520 #define T_ALIGN         17      // aligment check
3521 #define T_MCHK          18      // machine check
3522 #define T_SIMDERR       19      // SIMD floating point error
3523 
3524 // These are arbitrarily chosen, but with care not to overlap
3525 // processor defined exceptions or interrupt vectors.
3526 #define T_SYSCALL       64      // system call
3527 #define T_DEFAULT      500      // catchall
3528 
3529 #define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ
3530 
3531 #define IRQ_TIMER        0
3532 #define IRQ_KBD          1
3533 #define IRQ_COM1         4
3534 #define IRQ_IDE         14
3535 #define IRQ_ERROR       19
3536 #define IRQ_SPURIOUS    31
3537 
3538 
3539 
3540 
3541 
3542 
3543 
3544 
3545 
3546 
3547 
3548 
3549 
