3600 #include "mmu.h"
3601 
3602   # vectors.S sends all traps here.
3603 .globl alltraps
3604 alltraps:
3605   # Build trap frame.
3606   pushl %ds
3607   pushl %es
3608   pushl %fs
3609   pushl %gs
3610   pushal
3611 
3612   # Set up data segments.
3613   movw $(SEG_KDATA<<3), %ax
3614   movw %ax, %ds
3615   movw %ax, %es
3616 
3617   # Call trap(tf), where tf=%esp
3618   pushl %esp
3619   call trap
3620   addl $4, %esp
3621 
3622   # Return falls through to trapret...
3623 .globl trapret
3624 trapret:
3625   popal
3626   popl %gs
3627   popl %fs
3628   popl %es
3629   popl %ds
3630   addl $0x8, %esp  # trapno and errcode
3631   iret
3632 
3633 
3634 
3635 
3636 
3637 
3638 
3639 
3640 
3641 
3642 
3643 
3644 
3645 
3646 
3647 
3648 
3649 
