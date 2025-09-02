8800 #include "syscall.h"
8801 #include "traps.h"
8802 
8803 #define SYSCALL(name) \
8804   .globl name; \
8805   name: \
8806     movl $SYS_ ## name, %eax; \
8807     int $T_SYSCALL; \
8808     ret
8809 
8810 SYSCALL(fork)
8811 SYSCALL(exit)
8812 SYSCALL(wait)
8813 SYSCALL(pipe)
8814 SYSCALL(read)
8815 SYSCALL(write)
8816 SYSCALL(close)
8817 SYSCALL(kill)
8818 SYSCALL(exec)
8819 SYSCALL(open)
8820 SYSCALL(mknod)
8821 SYSCALL(unlink)
8822 SYSCALL(fstat)
8823 SYSCALL(link)
8824 SYSCALL(mkdir)
8825 SYSCALL(chdir)
8826 SYSCALL(dup)
8827 SYSCALL(getpid)
8828 SYSCALL(sbrk)
8829 SYSCALL(sleep)
8830 SYSCALL(uptime)
8831 
8832 SYSCALL(greet)
8833 SYSCALL(numvp)
8834 SYSCALL(numpp)
8835 SYSCALL(getptsize)
8836 SYSCALL(mmap)
8837 SYSCALL(munmap)
8838 SYSCALL(shm_open)
8839 SYSCALL(shm_get)
8840 SYSCALL(shm_close)
8841 
8842 
8843 
8844 
8845 
8846 
8847 
8848 
8849 
