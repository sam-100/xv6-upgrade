3650 #include "types.h"
3651 #include "defs.h"
3652 #include "param.h"
3653 #include "memlayout.h"
3654 #include "mmu.h"
3655 #include "proc.h"
3656 #include "x86.h"
3657 #include "traps.h"
3658 #include "spinlock.h"
3659 
3660 // Interrupt descriptor table (shared by all CPUs).
3661 struct gatedesc idt[256];
3662 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3663 struct spinlock tickslock;
3664 uint ticks;
3665 
3666 void
3667 tvinit(void)
3668 {
3669   int i;
3670 
3671   for(i = 0; i < 256; i++)
3672     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3673   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3674 
3675   initlock(&tickslock, "time");
3676 }
3677 
3678 void
3679 idtinit(void)
3680 {
3681   lidt(idt, sizeof(idt));
3682 }
3683 
3684 static int is_page_lazy(char *addr) {
3685   pde_t *pgdir = myproc()->pgdir;
3686   if(pgdir[PDX(addr)] & PTE_P == 0)
3687     return 0;
3688 
3689   pte_t *pgtable = P2V(PTE_ADDR(pgdir[PDX(addr)]));
3690   return ((pgtable[PTX(addr)] & PTE_LAZY) != 0);
3691 }
3692 
3693 
3694 
3695 
3696 
3697 
3698 
3699 
3700 void
3701 trap(struct trapframe *tf)
3702 {
3703   if(tf->trapno == T_SYSCALL){
3704     if(myproc()->killed)
3705       exit();
3706     myproc()->tf = tf;
3707     syscall();
3708     if(myproc()->killed)
3709       exit();
3710     return;
3711   }
3712 
3713   if(tf->trapno == T_PGFLT) {
3714     struct proc *p = myproc();
3715     uint addr = rcr2();
3716 
3717     if(!is_page_lazy(addr))
3718       goto kill_process;
3719 
3720     pde_t *pgdir = p->pgdir;
3721     pte_t *pgtable = P2V(PTE_ADDR(pgdir[PDX(addr)]));
3722     pgtable[PTX(addr)] = 0;
3723     uint mem = (uint)kalloc();
3724     if(mem == 0) {
3725       cprintf("kalloc: out of physical memory!\n");
3726       goto kill_process;
3727     }
3728 
3729     pgtable[PTX(addr)] &= ~(PTE_LAZY);
3730     pgtable[PTX(addr)] = V2P(mem) | PTE_P | PTE_W | PTE_U;
3731     return;
3732 
3733 
3734   kill_process:
3735     cprintf("Page fault: Killing process %d\n", p->pid);
3736     exit();
3737   }
3738 
3739   switch(tf->trapno){
3740   case T_IRQ0 + IRQ_TIMER:
3741     if(cpuid() == 0){
3742       acquire(&tickslock);
3743       ticks++;
3744       wakeup(&ticks);
3745       release(&tickslock);
3746     }
3747     lapiceoi();
3748     break;
3749   case T_IRQ0 + IRQ_IDE:
3750     ideintr();
3751     lapiceoi();
3752     break;
3753   case T_IRQ0 + IRQ_IDE+1:
3754     // Bochs generates spurious IDE1 interrupts.
3755     break;
3756   case T_IRQ0 + IRQ_KBD:
3757     kbdintr();
3758     lapiceoi();
3759     break;
3760   case T_IRQ0 + IRQ_COM1:
3761     uartintr();
3762     lapiceoi();
3763     break;
3764   case T_IRQ0 + 7:
3765   case T_IRQ0 + IRQ_SPURIOUS:
3766     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3767             cpuid(), tf->cs, tf->eip);
3768     lapiceoi();
3769     break;
3770 
3771 
3772   default:
3773     if(myproc() == 0 || (tf->cs&3) == 0){
3774       // In kernel, it must be our mistake.
3775       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3776               tf->trapno, cpuid(), tf->eip, rcr2());
3777       panic("trap");
3778     }
3779     // In user space, assume process misbehaved.
3780     cprintf("pid %d %s: trap %d err %d on cpu %d "
3781             "eip 0x%x addr 0x%x--kill proc\n",
3782             myproc()->pid, myproc()->name, tf->trapno,
3783             tf->err, cpuid(), tf->eip, rcr2());
3784     myproc()->killed = 1;
3785   }
3786 
3787   // Force process exit if it has been killed and is in user space.
3788   // (If it is still executing in the kernel, let it keep running
3789   // until it gets to the regular system call return.)
3790   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3791     exit();
3792 
3793   // Force process to give up CPU on clock tick.
3794   // If interrupts were on while locks held, would need to check nlock.
3795   if(myproc() && myproc()->state == RUNNING &&
3796      tf->trapno == T_IRQ0+IRQ_TIMER)
3797     yield();
3798 
3799 
3800   // Check if the process has been killed since we yielded
3801   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3802     exit();
3803 }
3804 
3805 
3806 
3807 
3808 
3809 
3810 
3811 
3812 
3813 
3814 
3815 
3816 
3817 
3818 
3819 
3820 
3821 
3822 
3823 
3824 
3825 
3826 
3827 
3828 
3829 
3830 
3831 
3832 
3833 
3834 
3835 
3836 
3837 
3838 
3839 
3840 
3841 
3842 
3843 
3844 
3845 
3846 
3847 
3848 
3849 
