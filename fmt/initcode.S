8750 # Initial process execs /init.
8751 # This code runs in user space.
8752 
8753 #include "syscall.h"
8754 #include "traps.h"
8755 
8756 
8757 # exec(init, argv)
8758 .globl start
8759 start:
8760   pushl $argv
8761   pushl $init
8762   pushl $0  // where caller pc would be
8763   movl $SYS_exec, %eax
8764   int $T_SYSCALL
8765 
8766 # for(;;) exit();
8767 exit:
8768   movl $SYS_exit, %eax
8769   int $T_SYSCALL
8770   jmp exit
8771 
8772 # char init[] = "/init\0";
8773 init:
8774   .string "/init\0"
8775 
8776 # char *argv[] = { init, 0 };
8777 .p2align 2
8778 argv:
8779   .long init
8780   .long 0
8781 
8782 
8783 
8784 
8785 
8786 
8787 
8788 
8789 
8790 
8791 
8792 
8793 
8794 
8795 
8796 
8797 
8798 
8799 
