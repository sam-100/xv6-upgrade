4100 #include "types.h"
4101 #include "x86.h"
4102 #include "defs.h"
4103 #include "date.h"
4104 #include "param.h"
4105 #include "memlayout.h"
4106 #include "mmu.h"
4107 #include "proc.h"
4108 
4109 int
4110 sys_fork(void)
4111 {
4112   return fork();
4113 }
4114 
4115 int
4116 sys_exit(void)
4117 {
4118   exit();
4119   return 0;  // not reached
4120 }
4121 
4122 int
4123 sys_wait(void)
4124 {
4125   return wait();
4126 }
4127 
4128 int
4129 sys_kill(void)
4130 {
4131   int pid;
4132 
4133   if(argint(0, &pid) < 0)
4134     return -1;
4135   return kill(pid);
4136 }
4137 
4138 int
4139 sys_getpid(void)
4140 {
4141   return myproc()->pid;
4142 }
4143 
4144 
4145 
4146 
4147 
4148 
4149 
4150 int
4151 sys_sbrk(void)
4152 {
4153   int addr;
4154   int n;
4155 
4156   if(argint(0, &n) < 0)
4157     return -1;
4158   addr = myproc()->sz;
4159   if(growproc(n) < 0)
4160     return -1;
4161   return addr;
4162 }
4163 
4164 int
4165 sys_sleep(void)
4166 {
4167   int n;
4168   uint ticks0;
4169 
4170   if(argint(0, &n) < 0)
4171     return -1;
4172   acquire(&tickslock);
4173   ticks0 = ticks;
4174   while(ticks - ticks0 < n){
4175     if(myproc()->killed){
4176       release(&tickslock);
4177       return -1;
4178     }
4179     sleep(&ticks, &tickslock);
4180   }
4181   release(&tickslock);
4182   return 0;
4183 }
4184 
4185 // return how many clock tick interrupts have occurred
4186 // since start.
4187 int
4188 sys_uptime(void)
4189 {
4190   uint xticks;
4191 
4192   acquire(&tickslock);
4193   xticks = ticks;
4194   release(&tickslock);
4195   return xticks;
4196 }
4197 
4198 
4199 
