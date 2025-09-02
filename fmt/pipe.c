7100 #include "types.h"
7101 #include "defs.h"
7102 #include "param.h"
7103 #include "mmu.h"
7104 #include "proc.h"
7105 #include "fs.h"
7106 #include "spinlock.h"
7107 #include "sleeplock.h"
7108 #include "file.h"
7109 
7110 #define PIPESIZE 512
7111 
7112 struct pipe {
7113   struct spinlock lock;
7114   char data[PIPESIZE];
7115   uint nread;     // number of bytes read
7116   uint nwrite;    // number of bytes written
7117   int readopen;   // read fd is still open
7118   int writeopen;  // write fd is still open
7119 };
7120 
7121 int
7122 pipealloc(struct file **f0, struct file **f1)
7123 {
7124   struct pipe *p;
7125 
7126   p = 0;
7127   *f0 = *f1 = 0;
7128   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
7129     goto bad;
7130   if((p = (struct pipe*)kalloc()) == 0)
7131     goto bad;
7132   p->readopen = 1;
7133   p->writeopen = 1;
7134   p->nwrite = 0;
7135   p->nread = 0;
7136   initlock(&p->lock, "pipe");
7137   (*f0)->type = FD_PIPE;
7138   (*f0)->readable = 1;
7139   (*f0)->writable = 0;
7140   (*f0)->pipe = p;
7141   (*f1)->type = FD_PIPE;
7142   (*f1)->readable = 0;
7143   (*f1)->writable = 1;
7144   (*f1)->pipe = p;
7145   return 0;
7146 
7147 
7148 
7149 
7150  bad:
7151   if(p)
7152     kfree((char*)p);
7153   if(*f0)
7154     fileclose(*f0);
7155   if(*f1)
7156     fileclose(*f1);
7157   return -1;
7158 }
7159 
7160 void
7161 pipeclose(struct pipe *p, int writable)
7162 {
7163   acquire(&p->lock);
7164   if(writable){
7165     p->writeopen = 0;
7166     wakeup(&p->nread);
7167   } else {
7168     p->readopen = 0;
7169     wakeup(&p->nwrite);
7170   }
7171   if(p->readopen == 0 && p->writeopen == 0){
7172     release(&p->lock);
7173     kfree((char*)p);
7174   } else
7175     release(&p->lock);
7176 }
7177 
7178 
7179 int
7180 pipewrite(struct pipe *p, char *addr, int n)
7181 {
7182   int i;
7183 
7184   acquire(&p->lock);
7185   for(i = 0; i < n; i++){
7186     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
7187       if(p->readopen == 0 || myproc()->killed){
7188         release(&p->lock);
7189         return -1;
7190       }
7191       wakeup(&p->nread);
7192       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
7193     }
7194     p->data[p->nwrite++ % PIPESIZE] = addr[i];
7195   }
7196   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
7197   release(&p->lock);
7198   return n;
7199 }
7200 int
7201 piperead(struct pipe *p, char *addr, int n)
7202 {
7203   int i;
7204 
7205   acquire(&p->lock);
7206   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
7207     if(myproc()->killed){
7208       release(&p->lock);
7209       return -1;
7210     }
7211     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
7212   }
7213   for(i = 0; i < n; i++){  //DOC: piperead-copy
7214     if(p->nread == p->nwrite)
7215       break;
7216     addr[i] = p->data[p->nread++ % PIPESIZE];
7217   }
7218   wakeup(&p->nwrite);  //DOC: piperead-wakeup
7219   release(&p->lock);
7220   return i;
7221 }
7222 
7223 
7224 
7225 
7226 
7227 
7228 
7229 
7230 
7231 
7232 
7233 
7234 
7235 
7236 
7237 
7238 
7239 
7240 
7241 
7242 
7243 
7244 
7245 
7246 
7247 
7248 
7249 
