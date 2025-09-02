4500 struct file {
4501   enum { FD_NONE, FD_PIPE, FD_INODE } type;
4502   int ref; // reference count
4503   char readable;
4504   char writable;
4505   struct pipe *pipe;
4506   struct inode *ip;
4507   uint off;
4508 };
4509 
4510 
4511 // in-memory copy of an inode
4512 struct inode {
4513   uint dev;           // Device number
4514   uint inum;          // Inode number
4515   int ref;            // Reference count
4516   struct sleeplock lock; // protects everything below here
4517   int valid;          // inode has been read from disk?
4518 
4519   short type;         // copy of disk inode
4520   short major;
4521   short minor;
4522   short nlink;
4523   uint size;
4524   uint addrs[NDIRECT+1];
4525 };
4526 
4527 // table mapping major device number to
4528 // device functions
4529 struct devsw {
4530   int (*read)(struct inode*, char*, int);
4531   int (*write)(struct inode*, char*, int);
4532 };
4533 
4534 extern struct devsw devsw[];
4535 
4536 #define CONSOLE 1
4537 
4538 
4539 
4540 
4541 
4542 
4543 
4544 
4545 
4546 
4547 
4548 
4549 
