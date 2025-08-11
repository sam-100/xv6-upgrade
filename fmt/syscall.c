3800 #include "types.h"
3801 #include "defs.h"
3802 #include "param.h"
3803 #include "memlayout.h"
3804 #include "mmu.h"
3805 #include "proc.h"
3806 #include "x86.h"
3807 #include "syscall.h"
3808 
3809 // User code makes a system call with INT T_SYSCALL.
3810 // System call number in %eax.
3811 // Arguments on the stack, from the user call to the C
3812 // library system call function. The saved user %esp points
3813 // to a saved program counter, and then the first argument.
3814 
3815 // Fetch the int at addr from the current process.
3816 int
3817 fetchint(uint addr, int *ip)
3818 {
3819   struct proc *curproc = myproc();
3820 
3821   if(addr >= curproc->sz || addr+4 > curproc->sz)
3822     return -1;
3823   *ip = *(int*)(addr);
3824   return 0;
3825 }
3826 
3827 // Fetch the nul-terminated string at addr from the current process.
3828 // Doesn't actually copy the string - just sets *pp to point at it.
3829 // Returns length of string, not including nul.
3830 int
3831 fetchstr(uint addr, char **pp)
3832 {
3833   char *s, *ep;
3834   struct proc *curproc = myproc();
3835 
3836   if(addr >= curproc->sz)
3837     return -1;
3838   *pp = (char*)addr;
3839   ep = (char*)curproc->sz;
3840   for(s = *pp; s < ep; s++){
3841     if(*s == 0)
3842       return s - *pp;
3843   }
3844   return -1;
3845 }
3846 
3847 
3848 
3849 
3850 // Fetch the nth 32-bit system call argument.
3851 int
3852 argint(int n, int *ip)
3853 {
3854   return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
3855 }
3856 
3857 // Fetch the nth word-sized system call argument as a pointer
3858 // to a block of memory of size bytes.  Check that the pointer
3859 // lies within the process address space.
3860 int
3861 argptr(int n, char **pp, int size)
3862 {
3863   int i;
3864   struct proc *curproc = myproc();
3865 
3866   if(argint(n, &i) < 0)
3867     return -1;
3868   if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
3869     return -1;
3870   *pp = (char*)i;
3871   return 0;
3872 }
3873 
3874 // Fetch the nth word-sized system call argument as a string pointer.
3875 // Check that the pointer is valid and the string is nul-terminated.
3876 // (There is no shared writable memory, so the string can't change
3877 // between this check and being used by the kernel.)
3878 int
3879 argstr(int n, char **pp)
3880 {
3881   int addr;
3882   if(argint(n, &addr) < 0)
3883     return -1;
3884   return fetchstr(addr, pp);
3885 }
3886 
3887 
3888 
3889 
3890 
3891 
3892 
3893 
3894 
3895 
3896 
3897 
3898 
3899 
3900 extern int sys_chdir(void);
3901 extern int sys_close(void);
3902 extern int sys_dup(void);
3903 extern int sys_exec(void);
3904 extern int sys_exit(void);
3905 extern int sys_fork(void);
3906 extern int sys_fstat(void);
3907 extern int sys_getpid(void);
3908 extern int sys_kill(void);
3909 extern int sys_link(void);
3910 extern int sys_mkdir(void);
3911 extern int sys_mknod(void);
3912 extern int sys_open(void);
3913 extern int sys_pipe(void);
3914 extern int sys_read(void);
3915 extern int sys_sbrk(void);
3916 extern int sys_sleep(void);
3917 extern int sys_unlink(void);
3918 extern int sys_wait(void);
3919 extern int sys_write(void);
3920 extern int sys_uptime(void);
3921 
3922 // new system calls
3923 extern void sys_greet(void);
3924 extern int sys_numvp(void);
3925 extern int sys_numpp(void);
3926 extern int sys_getptsize(void);
3927 extern int sys_mmap(void);
3928 extern int sys_munmap(void);
3929 
3930 
3931 static int (*syscalls[])(void) = {
3932 [SYS_fork]    sys_fork,
3933 [SYS_exit]    sys_exit,
3934 [SYS_wait]    sys_wait,
3935 [SYS_pipe]    sys_pipe,
3936 [SYS_read]    sys_read,
3937 [SYS_kill]    sys_kill,
3938 [SYS_exec]    sys_exec,
3939 [SYS_fstat]   sys_fstat,
3940 [SYS_chdir]   sys_chdir,
3941 [SYS_dup]     sys_dup,
3942 [SYS_getpid]  sys_getpid,
3943 [SYS_sbrk]    sys_sbrk,
3944 [SYS_sleep]   sys_sleep,
3945 [SYS_uptime]  sys_uptime,
3946 [SYS_open]    sys_open,
3947 [SYS_write]   sys_write,
3948 [SYS_mknod]   sys_mknod,
3949 [SYS_unlink]  sys_unlink,
3950 [SYS_link]    sys_link,
3951 [SYS_mkdir]   sys_mkdir,
3952 [SYS_close]   sys_close,
3953 [SYS_greet]   sys_greet,
3954 [SYS_numvp]   sys_numvp,
3955 [SYS_numpp]   sys_numpp,
3956 [SYS_getptsize]   sys_getptsize,
3957 [SYS_mmap]    sys_mmap,
3958 [SYS_munmap]  sys_munmap,
3959 };
3960 
3961 void
3962 syscall(void)
3963 {
3964   int num;
3965   struct proc *curproc = myproc();
3966 
3967   num = curproc->tf->eax;
3968   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
3969     curproc->tf->eax = syscalls[num]();
3970   } else {
3971     cprintf("%d %s: unknown sys call %d\n",
3972             curproc->pid, curproc->name, num);
3973     curproc->tf->eax = -1;
3974   }
3975 }
3976 
3977 
3978 
3979 
3980 
3981 
3982 
3983 
3984 
3985 
3986 
3987 
3988 
3989 
3990 
3991 
3992 
3993 
3994 
3995 
3996 
3997 
3998 
3999 
