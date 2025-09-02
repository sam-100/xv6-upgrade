4950 // Sleeping locks
4951 
4952 #include "types.h"
4953 #include "defs.h"
4954 #include "param.h"
4955 #include "x86.h"
4956 #include "memlayout.h"
4957 #include "mmu.h"
4958 #include "proc.h"
4959 #include "spinlock.h"
4960 #include "sleeplock.h"
4961 
4962 void
4963 initsleeplock(struct sleeplock *lk, char *name)
4964 {
4965   initlock(&lk->lk, "sleep lock");
4966   lk->name = name;
4967   lk->locked = 0;
4968   lk->pid = 0;
4969 }
4970 
4971 void
4972 acquiresleep(struct sleeplock *lk)
4973 {
4974   acquire(&lk->lk);
4975   while (lk->locked) {
4976     sleep(lk, &lk->lk);
4977   }
4978   lk->locked = 1;
4979   lk->pid = myproc()->pid;
4980   release(&lk->lk);
4981 }
4982 
4983 void
4984 releasesleep(struct sleeplock *lk)
4985 {
4986   acquire(&lk->lk);
4987   lk->locked = 0;
4988   lk->pid = 0;
4989   wakeup(lk);
4990   release(&lk->lk);
4991 }
4992 
4993 
4994 
4995 
4996 
4997 
4998 
4999 
5000 int
5001 holdingsleep(struct sleeplock *lk)
5002 {
5003   int r;
5004 
5005   acquire(&lk->lk);
5006   r = lk->locked && (lk->pid == myproc()->pid);
5007   release(&lk->lk);
5008   return r;
5009 }
5010 
5011 
5012 
5013 
5014 
5015 
5016 
5017 
5018 
5019 
5020 
5021 
5022 
5023 
5024 
5025 
5026 
5027 
5028 
5029 
5030 
5031 
5032 
5033 
5034 
5035 
5036 
5037 
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
