6400 //
6401 // File-system system calls.
6402 // Mostly argument checking, since we don't trust
6403 // user code, and calls into file.c and fs.c.
6404 //
6405 
6406 #include "types.h"
6407 #include "defs.h"
6408 #include "param.h"
6409 #include "stat.h"
6410 #include "mmu.h"
6411 #include "proc.h"
6412 #include "fs.h"
6413 #include "spinlock.h"
6414 #include "sleeplock.h"
6415 #include "file.h"
6416 #include "fcntl.h"
6417 
6418 // Fetch the nth word-sized system call argument as a file descriptor
6419 // and return both the descriptor and the corresponding struct file.
6420 static int
6421 argfd(int n, int *pfd, struct file **pf)
6422 {
6423   int fd;
6424   struct file *f;
6425 
6426   if(argint(n, &fd) < 0)
6427     return -1;
6428   if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
6429     return -1;
6430   if(pfd)
6431     *pfd = fd;
6432   if(pf)
6433     *pf = f;
6434   return 0;
6435 }
6436 
6437 
6438 
6439 
6440 
6441 
6442 
6443 
6444 
6445 
6446 
6447 
6448 
6449 
6450 // Allocate a file descriptor for the given file.
6451 // Takes over file reference from caller on success.
6452 static int
6453 fdalloc(struct file *f)
6454 {
6455   int fd;
6456   struct proc *curproc = myproc();
6457 
6458   for(fd = 0; fd < NOFILE; fd++){
6459     if(curproc->ofile[fd] == 0){
6460       curproc->ofile[fd] = f;
6461       return fd;
6462     }
6463   }
6464   return -1;
6465 }
6466 
6467 int
6468 sys_dup(void)
6469 {
6470   struct file *f;
6471   int fd;
6472 
6473   if(argfd(0, 0, &f) < 0)
6474     return -1;
6475   if((fd=fdalloc(f)) < 0)
6476     return -1;
6477   filedup(f);
6478   return fd;
6479 }
6480 
6481 int
6482 sys_read(void)
6483 {
6484   struct file *f;
6485   int n;
6486   char *p;
6487 
6488   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6489     return -1;
6490   return fileread(f, p, n);
6491 }
6492 
6493 
6494 
6495 
6496 
6497 
6498 
6499 
6500 int
6501 sys_write(void)
6502 {
6503   struct file *f;
6504   int n;
6505   char *p;
6506 
6507   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6508     return -1;
6509   return filewrite(f, p, n);
6510 }
6511 
6512 int
6513 sys_close(void)
6514 {
6515   int fd;
6516   struct file *f;
6517 
6518   if(argfd(0, &fd, &f) < 0)
6519     return -1;
6520   myproc()->ofile[fd] = 0;
6521   fileclose(f);
6522   return 0;
6523 }
6524 
6525 int
6526 sys_fstat(void)
6527 {
6528   struct file *f;
6529   struct stat *st;
6530 
6531   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
6532     return -1;
6533   return filestat(f, st);
6534 }
6535 
6536 
6537 
6538 
6539 
6540 
6541 
6542 
6543 
6544 
6545 
6546 
6547 
6548 
6549 
6550 // Create the path new as a link to the same inode as old.
6551 int
6552 sys_link(void)
6553 {
6554   char name[DIRSIZ], *new, *old;
6555   struct inode *dp, *ip;
6556 
6557   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
6558     return -1;
6559 
6560   begin_op();
6561   if((ip = namei(old)) == 0){
6562     end_op();
6563     return -1;
6564   }
6565 
6566   ilock(ip);
6567   if(ip->type == T_DIR){
6568     iunlockput(ip);
6569     end_op();
6570     return -1;
6571   }
6572 
6573   ip->nlink++;
6574   iupdate(ip);
6575   iunlock(ip);
6576 
6577   if((dp = nameiparent(new, name)) == 0)
6578     goto bad;
6579   ilock(dp);
6580   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
6581     iunlockput(dp);
6582     goto bad;
6583   }
6584   iunlockput(dp);
6585   iput(ip);
6586 
6587   end_op();
6588 
6589   return 0;
6590 
6591 bad:
6592   ilock(ip);
6593   ip->nlink--;
6594   iupdate(ip);
6595   iunlockput(ip);
6596   end_op();
6597   return -1;
6598 }
6599 
6600 // Is the directory dp empty except for "." and ".." ?
6601 static int
6602 isdirempty(struct inode *dp)
6603 {
6604   int off;
6605   struct dirent de;
6606 
6607   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
6608     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6609       panic("isdirempty: readi");
6610     if(de.inum != 0)
6611       return 0;
6612   }
6613   return 1;
6614 }
6615 
6616 
6617 
6618 
6619 
6620 
6621 
6622 
6623 
6624 
6625 
6626 
6627 
6628 
6629 
6630 
6631 
6632 
6633 
6634 
6635 
6636 
6637 
6638 
6639 
6640 
6641 
6642 
6643 
6644 
6645 
6646 
6647 
6648 
6649 
6650 int
6651 sys_unlink(void)
6652 {
6653   struct inode *ip, *dp;
6654   struct dirent de;
6655   char name[DIRSIZ], *path;
6656   uint off;
6657 
6658   if(argstr(0, &path) < 0)
6659     return -1;
6660 
6661   begin_op();
6662   if((dp = nameiparent(path, name)) == 0){
6663     end_op();
6664     return -1;
6665   }
6666 
6667   ilock(dp);
6668 
6669   // Cannot unlink "." or "..".
6670   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
6671     goto bad;
6672 
6673   if((ip = dirlookup(dp, name, &off)) == 0)
6674     goto bad;
6675   ilock(ip);
6676 
6677   if(ip->nlink < 1)
6678     panic("unlink: nlink < 1");
6679   if(ip->type == T_DIR && !isdirempty(ip)){
6680     iunlockput(ip);
6681     goto bad;
6682   }
6683 
6684   memset(&de, 0, sizeof(de));
6685   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6686     panic("unlink: writei");
6687   if(ip->type == T_DIR){
6688     dp->nlink--;
6689     iupdate(dp);
6690   }
6691   iunlockput(dp);
6692 
6693   ip->nlink--;
6694   iupdate(ip);
6695   iunlockput(ip);
6696 
6697   end_op();
6698 
6699   return 0;
6700 bad:
6701   iunlockput(dp);
6702   end_op();
6703   return -1;
6704 }
6705 
6706 static struct inode*
6707 create(char *path, short type, short major, short minor)
6708 {
6709   struct inode *ip, *dp;
6710   char name[DIRSIZ];
6711 
6712   if((dp = nameiparent(path, name)) == 0)
6713     return 0;
6714   ilock(dp);
6715 
6716   if((ip = dirlookup(dp, name, 0)) != 0){
6717     iunlockput(dp);
6718     ilock(ip);
6719     if(type == T_FILE && ip->type == T_FILE)
6720       return ip;
6721     iunlockput(ip);
6722     return 0;
6723   }
6724 
6725   if((ip = ialloc(dp->dev, type)) == 0)
6726     panic("create: ialloc");
6727 
6728   ilock(ip);
6729   ip->major = major;
6730   ip->minor = minor;
6731   ip->nlink = 1;
6732   iupdate(ip);
6733 
6734   if(type == T_DIR){  // Create . and .. entries.
6735     dp->nlink++;  // for ".."
6736     iupdate(dp);
6737     // No ip->nlink++ for ".": avoid cyclic ref count.
6738     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
6739       panic("create dots");
6740   }
6741 
6742   if(dirlink(dp, name, ip->inum) < 0)
6743     panic("create: dirlink");
6744 
6745   iunlockput(dp);
6746 
6747   return ip;
6748 }
6749 
6750 int
6751 sys_open(void)
6752 {
6753   char *path;
6754   int fd, omode;
6755   struct file *f;
6756   struct inode *ip;
6757 
6758   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
6759     return -1;
6760 
6761   begin_op();
6762 
6763   if(omode & O_CREATE){
6764     ip = create(path, T_FILE, 0, 0);
6765     if(ip == 0){
6766       end_op();
6767       return -1;
6768     }
6769   } else {
6770     if((ip = namei(path)) == 0){
6771       end_op();
6772       return -1;
6773     }
6774     ilock(ip);
6775     if(ip->type == T_DIR && omode != O_RDONLY){
6776       iunlockput(ip);
6777       end_op();
6778       return -1;
6779     }
6780   }
6781 
6782   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
6783     if(f)
6784       fileclose(f);
6785     iunlockput(ip);
6786     end_op();
6787     return -1;
6788   }
6789   iunlock(ip);
6790   end_op();
6791 
6792   f->type = FD_INODE;
6793   f->ip = ip;
6794   f->off = 0;
6795   f->readable = !(omode & O_WRONLY);
6796   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
6797   return fd;
6798 }
6799 
6800 int
6801 sys_mkdir(void)
6802 {
6803   char *path;
6804   struct inode *ip;
6805 
6806   begin_op();
6807   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
6808     end_op();
6809     return -1;
6810   }
6811   iunlockput(ip);
6812   end_op();
6813   return 0;
6814 }
6815 
6816 int
6817 sys_mknod(void)
6818 {
6819   struct inode *ip;
6820   char *path;
6821   int major, minor;
6822 
6823   begin_op();
6824   if((argstr(0, &path)) < 0 ||
6825      argint(1, &major) < 0 ||
6826      argint(2, &minor) < 0 ||
6827      (ip = create(path, T_DEV, major, minor)) == 0){
6828     end_op();
6829     return -1;
6830   }
6831   iunlockput(ip);
6832   end_op();
6833   return 0;
6834 }
6835 
6836 
6837 
6838 
6839 
6840 
6841 
6842 
6843 
6844 
6845 
6846 
6847 
6848 
6849 
6850 int
6851 sys_chdir(void)
6852 {
6853   char *path;
6854   struct inode *ip;
6855   struct proc *curproc = myproc();
6856 
6857   begin_op();
6858   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
6859     end_op();
6860     return -1;
6861   }
6862   ilock(ip);
6863   if(ip->type != T_DIR){
6864     iunlockput(ip);
6865     end_op();
6866     return -1;
6867   }
6868   iunlock(ip);
6869   iput(curproc->cwd);
6870   end_op();
6871   curproc->cwd = ip;
6872   return 0;
6873 }
6874 
6875 int
6876 sys_exec(void)
6877 {
6878   char *path, *argv[MAXARG];
6879   int i;
6880   uint uargv, uarg;
6881 
6882   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
6883     return -1;
6884   }
6885   memset(argv, 0, sizeof(argv));
6886   for(i=0;; i++){
6887     if(i >= NELEM(argv))
6888       return -1;
6889     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
6890       return -1;
6891     if(uarg == 0){
6892       argv[i] = 0;
6893       break;
6894     }
6895     if(fetchstr(uarg, &argv[i]) < 0)
6896       return -1;
6897   }
6898   return exec(path, argv);
6899 }
6900 int
6901 sys_pipe(void)
6902 {
6903   int *fd;
6904   struct file *rf, *wf;
6905   int fd0, fd1;
6906 
6907   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
6908     return -1;
6909   if(pipealloc(&rf, &wf) < 0)
6910     return -1;
6911   fd0 = -1;
6912   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
6913     if(fd0 >= 0)
6914       myproc()->ofile[fd0] = 0;
6915     fileclose(rf);
6916     fileclose(wf);
6917     return -1;
6918   }
6919   fd[0] = fd0;
6920   fd[1] = fd1;
6921   return 0;
6922 }
6923 
6924 
6925 
6926 
6927 
6928 
6929 
6930 
6931 
6932 
6933 
6934 
6935 
6936 
6937 
6938 
6939 
6940 
6941 
6942 
6943 
6944 
6945 
6946 
6947 
6948 
6949 
