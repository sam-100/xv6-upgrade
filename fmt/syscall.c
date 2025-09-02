3900 #include "types.h"
3901 #include "defs.h"
3902 #include "param.h"
3903 #include "memlayout.h"
3904 #include "mmu.h"
3905 #include "proc.h"
3906 #include "x86.h"
3907 #include "syscall.h"
3908 
3909 // User code makes a system call with INT T_SYSCALL.
3910 // System call number in %eax.
3911 // Arguments on the stack, from the user call to the C
3912 // library system call function. The saved user %esp points
3913 // to a saved program counter, and then the first argument.
3914 
3915 // Fetch the int at addr from the current process.
3916 int
3917 fetchint(uint addr, int *ip)
3918 {
3919   struct proc *curproc = myproc();
3920 
3921   if(addr >= curproc->sz || addr+4 > curproc->sz)
3922     return -1;
3923   *ip = *(int*)(addr);
3924   return 0;
3925 }
3926 
3927 // Fetch the nul-terminated string at addr from the current process.
3928 // Doesn't actually copy the string - just sets *pp to point at it.
3929 // Returns length of string, not including nul.
3930 int
3931 fetchstr(uint addr, char **pp)
3932 {
3933   char *s, *ep;
3934   struct proc *curproc = myproc();
3935 
3936   if(addr >= curproc->sz)
3937     return -1;
3938   *pp = (char*)addr;
3939   ep = (char*)curproc->sz;
3940   for(s = *pp; s < ep; s++){
3941     if(*s == 0)
3942       return s - *pp;
3943   }
3944   return -1;
3945 }
3946 
3947 
3948 
3949 
3950 // Fetch the nth 32-bit system call argument.
3951 int
3952 argint(int n, int *ip)
3953 {
3954   return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
3955 }
3956 
3957 // Fetch the nth word-sized system call argument as a pointer
3958 // to a block of memory of size bytes.  Check that the pointer
3959 // lies within the process address space.
3960 int
3961 argptr(int n, char **pp, int size)
3962 {
3963   int i;
3964   struct proc *curproc = myproc();
3965 
3966   if(argint(n, &i) < 0)
3967     return -1;
3968   if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
3969     return -1;
3970   *pp = (char*)i;
3971   return 0;
3972 }
3973 
3974 // Fetch the nth word-sized system call argument as a string pointer.
3975 // Check that the pointer is valid and the string is nul-terminated.
3976 // (There is no shared writable memory, so the string can't change
3977 // between this check and being used by the kernel.)
3978 int
3979 argstr(int n, char **pp)
3980 {
3981   int addr;
3982   if(argint(n, &addr) < 0)
3983     return -1;
3984   return fetchstr(addr, pp);
3985 }
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
4000 extern int sys_chdir(void);
4001 extern int sys_close(void);
4002 extern int sys_dup(void);
4003 extern int sys_exec(void);
4004 extern int sys_exit(void);
4005 extern int sys_fork(void);
4006 extern int sys_fstat(void);
4007 extern int sys_getpid(void);
4008 extern int sys_kill(void);
4009 extern int sys_link(void);
4010 extern int sys_mkdir(void);
4011 extern int sys_mknod(void);
4012 extern int sys_open(void);
4013 extern int sys_pipe(void);
4014 extern int sys_read(void);
4015 extern int sys_sbrk(void);
4016 extern int sys_sleep(void);
4017 extern int sys_unlink(void);
4018 extern int sys_wait(void);
4019 extern int sys_write(void);
4020 extern int sys_uptime(void);
4021 
4022 // new system calls
4023 extern void sys_greet(void);
4024 extern int sys_numvp(void);
4025 extern int sys_numpp(void);
4026 extern int sys_getptsize(void);
4027 extern int sys_mmap(void);
4028 extern int sys_munmap(void);
4029 extern uint sys_shm_open(void);
4030 extern uint sys_shm_get(void);
4031 extern int sys_shm_close(void);
4032 
4033 static int (*syscalls[])(void) = {
4034 [SYS_fork]    sys_fork,
4035 [SYS_exit]    sys_exit,
4036 [SYS_wait]    sys_wait,
4037 [SYS_pipe]    sys_pipe,
4038 [SYS_read]    sys_read,
4039 [SYS_kill]    sys_kill,
4040 [SYS_exec]    sys_exec,
4041 [SYS_fstat]   sys_fstat,
4042 [SYS_chdir]   sys_chdir,
4043 [SYS_dup]     sys_dup,
4044 [SYS_getpid]  sys_getpid,
4045 [SYS_sbrk]    sys_sbrk,
4046 [SYS_sleep]   sys_sleep,
4047 [SYS_uptime]  sys_uptime,
4048 [SYS_open]    sys_open,
4049 [SYS_write]   sys_write,
4050 [SYS_mknod]   sys_mknod,
4051 [SYS_unlink]  sys_unlink,
4052 [SYS_link]    sys_link,
4053 [SYS_mkdir]   sys_mkdir,
4054 [SYS_close]   sys_close,
4055 [SYS_greet]   sys_greet,
4056 [SYS_numvp]   sys_numvp,
4057 [SYS_numpp]   sys_numpp,
4058 [SYS_getptsize]   sys_getptsize,
4059 [SYS_mmap]    sys_mmap,
4060 [SYS_munmap]  sys_munmap,
4061 [SYS_shm_open]  sys_shm_open,
4062 [SYS_shm_get] sys_shm_get,
4063 [SYS_shm_close] sys_shm_close,
4064 };
4065 
4066 void
4067 syscall(void)
4068 {
4069   int num;
4070   struct proc *curproc = myproc();
4071 
4072   num = curproc->tf->eax;
4073   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
4074     curproc->tf->eax = syscalls[num]();
4075   } else {
4076     cprintf("%d %s: unknown sys call %d\n",
4077             curproc->pid, curproc->name, num);
4078     curproc->tf->eax = -1;
4079   }
4080 }
4081 
4082 
4083 
4084 
4085 
4086 
4087 
4088 
4089 
4090 
4091 
4092 
4093 
4094 
4095 
4096 
4097 
4098 
4099 
