6950 #include "types.h"
6951 #include "param.h"
6952 #include "memlayout.h"
6953 #include "mmu.h"
6954 #include "proc.h"
6955 #include "defs.h"
6956 #include "x86.h"
6957 #include "elf.h"
6958 
6959 int
6960 exec(char *path, char **argv)
6961 {
6962   char *s, *last;
6963   int i, off;
6964   uint argc, sz, sp, ustack[3+MAXARG+1];
6965   struct elfhdr elf;
6966   struct inode *ip;
6967   struct proghdr ph;
6968   pde_t *pgdir, *oldpgdir;
6969   struct proc *curproc = myproc();
6970 
6971   begin_op();
6972 
6973   if((ip = namei(path)) == 0){
6974     end_op();
6975     cprintf("exec: fail\n");
6976     return -1;
6977   }
6978   ilock(ip);
6979   pgdir = 0;
6980 
6981   // Check ELF header
6982   if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
6983     goto bad;
6984   if(elf.magic != ELF_MAGIC)
6985     goto bad;
6986 
6987   if((pgdir = setupkvm()) == 0)
6988     goto bad;
6989 
6990   // Load program into memory.
6991   sz = 0;
6992   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6993     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6994       goto bad;
6995     if(ph.type != ELF_PROG_LOAD)
6996       continue;
6997     if(ph.memsz < ph.filesz)
6998       goto bad;
6999     if(ph.vaddr + ph.memsz < ph.vaddr)
7000       goto bad;
7001     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
7002       goto bad;
7003     if(ph.vaddr % PGSIZE != 0)
7004       goto bad;
7005     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
7006       goto bad;
7007   }
7008   iunlockput(ip);
7009   end_op();
7010   ip = 0;
7011 
7012   // Allocate two pages at the next page boundary.
7013   // Make the first inaccessible.  Use the second as the user stack.
7014   sz = PGROUNDUP(sz);
7015   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
7016     goto bad;
7017   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
7018   sp = sz;
7019 
7020   // Push argument strings, prepare rest of stack in ustack.
7021   for(argc = 0; argv[argc]; argc++) {
7022     if(argc >= MAXARG)
7023       goto bad;
7024     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
7025     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
7026       goto bad;
7027     ustack[3+argc] = sp;
7028   }
7029   ustack[3+argc] = 0;
7030 
7031   ustack[0] = 0xffffffff;  // fake return PC
7032   ustack[1] = argc;
7033   ustack[2] = sp - (argc+1)*4;  // argv pointer
7034 
7035   sp -= (3+argc+1) * 4;
7036   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
7037     goto bad;
7038 
7039   // Save program name for debugging.
7040   for(last=s=path; *s; s++)
7041     if(*s == '/')
7042       last = s+1;
7043   safestrcpy(curproc->name, last, sizeof(curproc->name));
7044 
7045   // Commit to the user image.
7046   oldpgdir = curproc->pgdir;
7047   curproc->pgdir = pgdir;
7048   curproc->sz = sz;
7049   curproc->tf->eip = elf.entry;  // main
7050   curproc->tf->esp = sp;
7051   switchuvm(curproc);
7052   freevm(oldpgdir);
7053   return 0;
7054 
7055  bad:
7056   if(pgdir)
7057     freevm(pgdir);
7058   if(ip){
7059     iunlockput(ip);
7060     end_op();
7061   }
7062   return -1;
7063 }
7064 
7065 
7066 
7067 
7068 
7069 
7070 
7071 
7072 
7073 
7074 
7075 
7076 
7077 
7078 
7079 
7080 
7081 
7082 
7083 
7084 
7085 
7086 
7087 
7088 
7089 
7090 
7091 
7092 
7093 
7094 
7095 
7096 
7097 
7098 
7099 
