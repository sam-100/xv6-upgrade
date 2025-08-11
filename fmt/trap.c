3550 #include "types.h"
3551 #include "defs.h"
3552 #include "param.h"
3553 #include "memlayout.h"
3554 #include "mmu.h"
3555 #include "proc.h"
3556 #include "x86.h"
3557 #include "traps.h"
3558 #include "spinlock.h"
3559 
3560 // Interrupt descriptor table (shared by all CPUs).
3561 struct gatedesc idt[256];
3562 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3563 struct spinlock tickslock;
3564 uint ticks;
3565 
3566 void
3567 tvinit(void)
3568 {
3569   int i;
3570 
3571   for(i = 0; i < 256; i++)
3572     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3573   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3574 
3575   initlock(&tickslock, "time");
3576 }
3577 
3578 void
3579 idtinit(void)
3580 {
3581   lidt(idt, sizeof(idt));
3582 }
3583 
3584 static int is_page_lazy(char *addr) {
3585   pde_t *pgdir = myproc()->pgdir;
3586   if(pgdir[PDX(addr)] & PTE_P == 0)
3587     return 0;
3588 
3589   pte_t *pgtable = P2V(PTE_ADDR(pgdir[PDX(addr)]));
3590   return ((pgtable[PTX(addr)] & PTE_LAZY) != 0);
3591 }
3592 
3593 
3594 
3595 
3596 
3597 
3598 
3599 
3600 void
3601 trap(struct trapframe *tf)
3602 {
3603   if(tf->trapno == T_SYSCALL){
3604     if(myproc()->killed)
3605       exit();
3606     myproc()->tf = tf;
3607     syscall();
3608     if(myproc()->killed)
3609       exit();
3610     return;
3611   }
3612 
3613   if(tf->trapno == T_PGFLT) {
3614     struct proc *p = myproc();
3615     uint addr = rcr2();
3616 
3617     if(!is_page_lazy(addr))
3618       goto kill_process;
3619 
3620     pde_t *pgdir = p->pgdir;
3621     pte_t *pgtable = P2V(PTE_ADDR(pgdir[PDX(addr)]));
3622     pgtable[PTX(addr)] = 0;
3623     uint mem = (uint)kalloc();
3624     if(mem == 0) {
3625       cprintf("kalloc: out of physical memory!\n");
3626       goto kill_process;
3627     }
3628 
3629     pgtable[PTX(addr)] &= ~(PTE_LAZY);
3630     pgtable[PTX(addr)] = V2P(mem) | PTE_P | PTE_W | PTE_U;
3631     return;
3632 
3633 
3634   kill_process:
3635     cprintf("Page fault: Killing process %d\n", p->pid);
3636     exit();
3637   }
3638 
3639   switch(tf->trapno){
3640   case T_IRQ0 + IRQ_TIMER:
3641     if(cpuid() == 0){
3642       acquire(&tickslock);
3643       ticks++;
3644       wakeup(&ticks);
3645       release(&tickslock);
3646     }
3647     lapiceoi();
3648     break;
3649   case T_IRQ0 + IRQ_IDE:
3650     ideintr();
3651     lapiceoi();
3652     break;
3653   case T_IRQ0 + IRQ_IDE+1:
3654     // Bochs generates spurious IDE1 interrupts.
3655     break;
3656   case T_IRQ0 + IRQ_KBD:
3657     kbdintr();
3658     lapiceoi();
3659     break;
3660   case T_IRQ0 + IRQ_COM1:
3661     uartintr();
3662     lapiceoi();
3663     break;
3664   case T_IRQ0 + 7:
3665   case T_IRQ0 + IRQ_SPURIOUS:
3666     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3667             cpuid(), tf->cs, tf->eip);
3668     lapiceoi();
3669     break;
3670 
3671 
3672   default:
3673     if(myproc() == 0 || (tf->cs&3) == 0){
3674       // In kernel, it must be our mistake.
3675       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3676               tf->trapno, cpuid(), tf->eip, rcr2());
3677       panic("trap");
3678     }
3679     // In user space, assume process misbehaved.
3680     cprintf("pid %d %s: trap %d err %d on cpu %d "
3681             "eip 0x%x addr 0x%x--kill proc\n",
3682             myproc()->pid, myproc()->name, tf->trapno,
3683             tf->err, cpuid(), tf->eip, rcr2());
3684     myproc()->killed = 1;
3685   }
3686 
3687   // Force process exit if it has been killed and is in user space.
3688   // (If it is still executing in the kernel, let it keep running
3689   // until it gets to the regular system call return.)
3690   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3691     exit();
3692 
3693   // Force process to give up CPU on clock tick.
3694   // If interrupts were on while locks held, would need to check nlock.
3695   if(myproc() && myproc()->state == RUNNING &&
3696      tf->trapno == T_IRQ0+IRQ_TIMER)
3697     yield();
3698 
3699 
3700   // Check if the process has been killed since we yielded
3701   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3702     exit();
3703 }
3704 
3705 
3706 
3707 
3708 
3709 
3710 
3711 
3712 
3713 
3714 
3715 
3716 
3717 
3718 
3719 
3720 
3721 
3722 
3723 
3724 
3725 
3726 
3727 
3728 
3729 
3730 
3731 
3732 
3733 
3734 
3735 
3736 
3737 
3738 
3739 
3740 
3741 
3742 
3743 
3744 
3745 
3746 
3747 
3748 
3749 
