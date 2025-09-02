4400 // On-disk file system format.
4401 // Both the kernel and user programs use this header file.
4402 
4403 
4404 #define ROOTINO 1  // root i-number
4405 #define BSIZE 512  // block size
4406 
4407 // Disk layout:
4408 // [ boot block | super block | log | inode blocks |
4409 //                                          free bit map | data blocks]
4410 //
4411 // mkfs computes the super block and builds an initial file system. The
4412 // super block describes the disk layout:
4413 struct superblock {
4414   uint size;         // Size of file system image (blocks)
4415   uint nblocks;      // Number of data blocks
4416   uint ninodes;      // Number of inodes.
4417   uint nlog;         // Number of log blocks
4418   uint logstart;     // Block number of first log block
4419   uint inodestart;   // Block number of first inode block
4420   uint bmapstart;    // Block number of first free map block
4421 };
4422 
4423 #define NDIRECT 12
4424 #define NINDIRECT (BSIZE / sizeof(uint))
4425 #define MAXFILE (NDIRECT + NINDIRECT)
4426 
4427 // On-disk inode structure
4428 struct dinode {
4429   short type;           // File type
4430   short major;          // Major device number (T_DEV only)
4431   short minor;          // Minor device number (T_DEV only)
4432   short nlink;          // Number of links to inode in file system
4433   uint size;            // Size of file (bytes)
4434   uint addrs[NDIRECT+1];   // Data block addresses
4435 };
4436 
4437 
4438 
4439 
4440 
4441 
4442 
4443 
4444 
4445 
4446 
4447 
4448 
4449 
4450 // Inodes per block.
4451 #define IPB           (BSIZE / sizeof(struct dinode))
4452 
4453 // Block containing inode i
4454 #define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
4455 
4456 // Bitmap bits per block
4457 #define BPB           (BSIZE*8)
4458 
4459 // Block of free map containing bit for block b
4460 #define BBLOCK(b, sb) (b/BPB + sb.bmapstart)
4461 
4462 // Directory is a file containing a sequence of dirent structures.
4463 #define DIRSIZ 14
4464 
4465 struct dirent {
4466   ushort inum;
4467   char name[DIRSIZ];
4468 };
4469 
4470 
4471 
4472 
4473 
4474 
4475 
4476 
4477 
4478 
4479 
4480 
4481 
4482 
4483 
4484 
4485 
4486 
4487 
4488 
4489 
4490 
4491 
4492 
4493 
4494 
4495 
4496 
4497 
4498 
4499 
