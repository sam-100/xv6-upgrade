5300 // File system implementation.  Five layers:
5301 //   + Blocks: allocator for raw disk blocks.
5302 //   + Log: crash recovery for multi-step updates.
5303 //   + Files: inode allocator, reading, writing, metadata.
5304 //   + Directories: inode with special contents (list of other inodes!)
5305 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
5306 //
5307 // This file contains the low-level file system manipulation
5308 // routines.  The (higher-level) system call implementations
5309 // are in sysfile.c.
5310 
5311 #include "types.h"
5312 #include "defs.h"
5313 #include "param.h"
5314 #include "stat.h"
5315 #include "mmu.h"
5316 #include "proc.h"
5317 #include "spinlock.h"
5318 #include "sleeplock.h"
5319 #include "fs.h"
5320 #include "buf.h"
5321 #include "file.h"
5322 
5323 #define min(a, b) ((a) < (b) ? (a) : (b))
5324 static void itrunc(struct inode*);
5325 // there should be one superblock per disk device, but we run with
5326 // only one device
5327 struct superblock sb;
5328 
5329 // Read the super block.
5330 void
5331 readsb(int dev, struct superblock *sb)
5332 {
5333   struct buf *bp;
5334 
5335   bp = bread(dev, 1);
5336   memmove(sb, bp->data, sizeof(*sb));
5337   brelse(bp);
5338 }
5339 
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Zero a block.
5351 static void
5352 bzero(int dev, int bno)
5353 {
5354   struct buf *bp;
5355 
5356   bp = bread(dev, bno);
5357   memset(bp->data, 0, BSIZE);
5358   log_write(bp);
5359   brelse(bp);
5360 }
5361 
5362 // Blocks.
5363 
5364 // Allocate a zeroed disk block.
5365 static uint
5366 balloc(uint dev)
5367 {
5368   int b, bi, m;
5369   struct buf *bp;
5370 
5371   bp = 0;
5372   for(b = 0; b < sb.size; b += BPB){
5373     bp = bread(dev, BBLOCK(b, sb));
5374     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
5375       m = 1 << (bi % 8);
5376       if((bp->data[bi/8] & m) == 0){  // Is block free?
5377         bp->data[bi/8] |= m;  // Mark block in use.
5378         log_write(bp);
5379         brelse(bp);
5380         bzero(dev, b + bi);
5381         return b + bi;
5382       }
5383     }
5384     brelse(bp);
5385   }
5386   panic("balloc: out of blocks");
5387 }
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Free a disk block.
5401 static void
5402 bfree(int dev, uint b)
5403 {
5404   struct buf *bp;
5405   int bi, m;
5406 
5407   bp = bread(dev, BBLOCK(b, sb));
5408   bi = b % BPB;
5409   m = 1 << (bi % 8);
5410   if((bp->data[bi/8] & m) == 0)
5411     panic("freeing free block");
5412   bp->data[bi/8] &= ~m;
5413   log_write(bp);
5414   brelse(bp);
5415 }
5416 
5417 // Inodes.
5418 //
5419 // An inode describes a single unnamed file.
5420 // The inode disk structure holds metadata: the file's type,
5421 // its size, the number of links referring to it, and the
5422 // list of blocks holding the file's content.
5423 //
5424 // The inodes are laid out sequentially on disk at
5425 // sb.startinode. Each inode has a number, indicating its
5426 // position on the disk.
5427 //
5428 // The kernel keeps a cache of in-use inodes in memory
5429 // to provide a place for synchronizing access
5430 // to inodes used by multiple processes. The cached
5431 // inodes include book-keeping information that is
5432 // not stored on disk: ip->ref and ip->valid.
5433 //
5434 // An inode and its in-memory representation go through a
5435 // sequence of states before they can be used by the
5436 // rest of the file system code.
5437 //
5438 // * Allocation: an inode is allocated if its type (on disk)
5439 //   is non-zero. ialloc() allocates, and iput() frees if
5440 //   the reference and link counts have fallen to zero.
5441 //
5442 // * Referencing in cache: an entry in the inode cache
5443 //   is free if ip->ref is zero. Otherwise ip->ref tracks
5444 //   the number of in-memory pointers to the entry (open
5445 //   files and current directories). iget() finds or
5446 //   creates a cache entry and increments its ref; iput()
5447 //   decrements ref.
5448 //
5449 // * Valid: the information (type, size, &c) in an inode
5450 //   cache entry is only correct when ip->valid is 1.
5451 //   ilock() reads the inode from
5452 //   the disk and sets ip->valid, while iput() clears
5453 //   ip->valid if ip->ref has fallen to zero.
5454 //
5455 // * Locked: file system code may only examine and modify
5456 //   the information in an inode and its content if it
5457 //   has first locked the inode.
5458 //
5459 // Thus a typical sequence is:
5460 //   ip = iget(dev, inum)
5461 //   ilock(ip)
5462 //   ... examine and modify ip->xxx ...
5463 //   iunlock(ip)
5464 //   iput(ip)
5465 //
5466 // ilock() is separate from iget() so that system calls can
5467 // get a long-term reference to an inode (as for an open file)
5468 // and only lock it for short periods (e.g., in read()).
5469 // The separation also helps avoid deadlock and races during
5470 // pathname lookup. iget() increments ip->ref so that the inode
5471 // stays cached and pointers to it remain valid.
5472 //
5473 // Many internal file system functions expect the caller to
5474 // have locked the inodes involved; this lets callers create
5475 // multi-step atomic operations.
5476 //
5477 // The icache.lock spin-lock protects the allocation of icache
5478 // entries. Since ip->ref indicates whether an entry is free,
5479 // and ip->dev and ip->inum indicate which i-node an entry
5480 // holds, one must hold icache.lock while using any of those fields.
5481 //
5482 // An ip->lock sleep-lock protects all ip-> fields other than ref,
5483 // dev, and inum.  One must hold ip->lock in order to
5484 // read or write that inode's ip->valid, ip->size, ip->type, &c.
5485 
5486 struct {
5487   struct spinlock lock;
5488   struct inode inode[NINODE];
5489 } icache;
5490 
5491 void
5492 iinit(int dev)
5493 {
5494   int i = 0;
5495 
5496   initlock(&icache.lock, "icache");
5497   for(i = 0; i < NINODE; i++) {
5498     initsleeplock(&icache.inode[i].lock, "inode");
5499   }
5500   readsb(dev, &sb);
5501   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
5502  inodestart %d bmap start %d\n", sb.size, sb.nblocks,
5503           sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
5504           sb.bmapstart);
5505 }
5506 
5507 static struct inode* iget(uint dev, uint inum);
5508 
5509 
5510 
5511 
5512 
5513 
5514 
5515 
5516 
5517 
5518 
5519 
5520 
5521 
5522 
5523 
5524 
5525 
5526 
5527 
5528 
5529 
5530 
5531 
5532 
5533 
5534 
5535 
5536 
5537 
5538 
5539 
5540 
5541 
5542 
5543 
5544 
5545 
5546 
5547 
5548 
5549 
5550 // Allocate an inode on device dev.
5551 // Mark it as allocated by  giving it type type.
5552 // Returns an unlocked but allocated and referenced inode.
5553 struct inode*
5554 ialloc(uint dev, short type)
5555 {
5556   int inum;
5557   struct buf *bp;
5558   struct dinode *dip;
5559 
5560   for(inum = 1; inum < sb.ninodes; inum++){
5561     bp = bread(dev, IBLOCK(inum, sb));
5562     dip = (struct dinode*)bp->data + inum%IPB;
5563     if(dip->type == 0){  // a free inode
5564       memset(dip, 0, sizeof(*dip));
5565       dip->type = type;
5566       log_write(bp);   // mark it allocated on the disk
5567       brelse(bp);
5568       return iget(dev, inum);
5569     }
5570     brelse(bp);
5571   }
5572   panic("ialloc: no inodes");
5573 }
5574 
5575 // Copy a modified in-memory inode to disk.
5576 // Must be called after every change to an ip->xxx field
5577 // that lives on disk, since i-node cache is write-through.
5578 // Caller must hold ip->lock.
5579 void
5580 iupdate(struct inode *ip)
5581 {
5582   struct buf *bp;
5583   struct dinode *dip;
5584 
5585   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5586   dip = (struct dinode*)bp->data + ip->inum%IPB;
5587   dip->type = ip->type;
5588   dip->major = ip->major;
5589   dip->minor = ip->minor;
5590   dip->nlink = ip->nlink;
5591   dip->size = ip->size;
5592   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
5593   log_write(bp);
5594   brelse(bp);
5595 }
5596 
5597 
5598 
5599 
5600 // Find the inode with number inum on device dev
5601 // and return the in-memory copy. Does not lock
5602 // the inode and does not read it from disk.
5603 static struct inode*
5604 iget(uint dev, uint inum)
5605 {
5606   struct inode *ip, *empty;
5607 
5608   acquire(&icache.lock);
5609 
5610   // Is the inode already cached?
5611   empty = 0;
5612   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
5613     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
5614       ip->ref++;
5615       release(&icache.lock);
5616       return ip;
5617     }
5618     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
5619       empty = ip;
5620   }
5621 
5622   // Recycle an inode cache entry.
5623   if(empty == 0)
5624     panic("iget: no inodes");
5625 
5626   ip = empty;
5627   ip->dev = dev;
5628   ip->inum = inum;
5629   ip->ref = 1;
5630   ip->valid = 0;
5631   release(&icache.lock);
5632 
5633   return ip;
5634 }
5635 
5636 // Increment reference count for ip.
5637 // Returns ip to enable ip = idup(ip1) idiom.
5638 struct inode*
5639 idup(struct inode *ip)
5640 {
5641   acquire(&icache.lock);
5642   ip->ref++;
5643   release(&icache.lock);
5644   return ip;
5645 }
5646 
5647 
5648 
5649 
5650 // Lock the given inode.
5651 // Reads the inode from disk if necessary.
5652 void
5653 ilock(struct inode *ip)
5654 {
5655   struct buf *bp;
5656   struct dinode *dip;
5657 
5658   if(ip == 0 || ip->ref < 1)
5659     panic("ilock");
5660 
5661   acquiresleep(&ip->lock);
5662 
5663   if(ip->valid == 0){
5664     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5665     dip = (struct dinode*)bp->data + ip->inum%IPB;
5666     ip->type = dip->type;
5667     ip->major = dip->major;
5668     ip->minor = dip->minor;
5669     ip->nlink = dip->nlink;
5670     ip->size = dip->size;
5671     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
5672     brelse(bp);
5673     ip->valid = 1;
5674     if(ip->type == 0)
5675       panic("ilock: no type");
5676   }
5677 }
5678 
5679 // Unlock the given inode.
5680 void
5681 iunlock(struct inode *ip)
5682 {
5683   if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
5684     panic("iunlock");
5685 
5686   releasesleep(&ip->lock);
5687 }
5688 
5689 
5690 
5691 
5692 
5693 
5694 
5695 
5696 
5697 
5698 
5699 
5700 // Drop a reference to an in-memory inode.
5701 // If that was the last reference, the inode cache entry can
5702 // be recycled.
5703 // If that was the last reference and the inode has no links
5704 // to it, free the inode (and its content) on disk.
5705 // All calls to iput() must be inside a transaction in
5706 // case it has to free the inode.
5707 void
5708 iput(struct inode *ip)
5709 {
5710   acquiresleep(&ip->lock);
5711   if(ip->valid && ip->nlink == 0){
5712     acquire(&icache.lock);
5713     int r = ip->ref;
5714     release(&icache.lock);
5715     if(r == 1){
5716       // inode has no links and no other references: truncate and free.
5717       itrunc(ip);
5718       ip->type = 0;
5719       iupdate(ip);
5720       ip->valid = 0;
5721     }
5722   }
5723   releasesleep(&ip->lock);
5724 
5725   acquire(&icache.lock);
5726   ip->ref--;
5727   release(&icache.lock);
5728 }
5729 
5730 // Common idiom: unlock, then put.
5731 void
5732 iunlockput(struct inode *ip)
5733 {
5734   iunlock(ip);
5735   iput(ip);
5736 }
5737 
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
5750 // Inode content
5751 //
5752 // The content (data) associated with each inode is stored
5753 // in blocks on the disk. The first NDIRECT block numbers
5754 // are listed in ip->addrs[].  The next NINDIRECT blocks are
5755 // listed in block ip->addrs[NDIRECT].
5756 
5757 // Return the disk block address of the nth block in inode ip.
5758 // If there is no such block, bmap allocates one.
5759 static uint
5760 bmap(struct inode *ip, uint bn)
5761 {
5762   uint addr, *a;
5763   struct buf *bp;
5764 
5765   if(bn < NDIRECT){
5766     if((addr = ip->addrs[bn]) == 0)
5767       ip->addrs[bn] = addr = balloc(ip->dev);
5768     return addr;
5769   }
5770   bn -= NDIRECT;
5771 
5772   if(bn < NINDIRECT){
5773     // Load indirect block, allocating if necessary.
5774     if((addr = ip->addrs[NDIRECT]) == 0)
5775       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
5776     bp = bread(ip->dev, addr);
5777     a = (uint*)bp->data;
5778     if((addr = a[bn]) == 0){
5779       a[bn] = addr = balloc(ip->dev);
5780       log_write(bp);
5781     }
5782     brelse(bp);
5783     return addr;
5784   }
5785 
5786   panic("bmap: out of range");
5787 }
5788 
5789 
5790 
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 // Truncate inode (discard contents).
5801 // Only called when the inode has no links
5802 // to it (no directory entries referring to it)
5803 // and has no in-memory reference to it (is
5804 // not an open file or current directory).
5805 static void
5806 itrunc(struct inode *ip)
5807 {
5808   int i, j;
5809   struct buf *bp;
5810   uint *a;
5811 
5812   for(i = 0; i < NDIRECT; i++){
5813     if(ip->addrs[i]){
5814       bfree(ip->dev, ip->addrs[i]);
5815       ip->addrs[i] = 0;
5816     }
5817   }
5818 
5819   if(ip->addrs[NDIRECT]){
5820     bp = bread(ip->dev, ip->addrs[NDIRECT]);
5821     a = (uint*)bp->data;
5822     for(j = 0; j < NINDIRECT; j++){
5823       if(a[j])
5824         bfree(ip->dev, a[j]);
5825     }
5826     brelse(bp);
5827     bfree(ip->dev, ip->addrs[NDIRECT]);
5828     ip->addrs[NDIRECT] = 0;
5829   }
5830 
5831   ip->size = 0;
5832   iupdate(ip);
5833 }
5834 
5835 // Copy stat information from inode.
5836 // Caller must hold ip->lock.
5837 void
5838 stati(struct inode *ip, struct stat *st)
5839 {
5840   st->dev = ip->dev;
5841   st->ino = ip->inum;
5842   st->type = ip->type;
5843   st->nlink = ip->nlink;
5844   st->size = ip->size;
5845 }
5846 
5847 
5848 
5849 
5850 // Read data from inode.
5851 // Caller must hold ip->lock.
5852 int
5853 readi(struct inode *ip, char *dst, uint off, uint n)
5854 {
5855   uint tot, m;
5856   struct buf *bp;
5857 
5858   if(ip->type == T_DEV){
5859     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5860       return -1;
5861     return devsw[ip->major].read(ip, dst, n);
5862   }
5863 
5864   if(off > ip->size || off + n < off)
5865     return -1;
5866   if(off + n > ip->size)
5867     n = ip->size - off;
5868 
5869   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5870     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5871     m = min(n - tot, BSIZE - off%BSIZE);
5872     memmove(dst, bp->data + off%BSIZE, m);
5873     brelse(bp);
5874   }
5875   return n;
5876 }
5877 
5878 
5879 
5880 
5881 
5882 
5883 
5884 
5885 
5886 
5887 
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
5900 // Write data to inode.
5901 // Caller must hold ip->lock.
5902 int
5903 writei(struct inode *ip, char *src, uint off, uint n)
5904 {
5905   uint tot, m;
5906   struct buf *bp;
5907 
5908   if(ip->type == T_DEV){
5909     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5910       return -1;
5911     return devsw[ip->major].write(ip, src, n);
5912   }
5913 
5914   if(off > ip->size || off + n < off)
5915     return -1;
5916   if(off + n > MAXFILE*BSIZE)
5917     return -1;
5918 
5919   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5920     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5921     m = min(n - tot, BSIZE - off%BSIZE);
5922     memmove(bp->data + off%BSIZE, src, m);
5923     log_write(bp);
5924     brelse(bp);
5925   }
5926 
5927   if(n > 0 && off > ip->size){
5928     ip->size = off;
5929     iupdate(ip);
5930   }
5931   return n;
5932 }
5933 
5934 
5935 
5936 
5937 
5938 
5939 
5940 
5941 
5942 
5943 
5944 
5945 
5946 
5947 
5948 
5949 
5950 // Directories
5951 
5952 int
5953 namecmp(const char *s, const char *t)
5954 {
5955   return strncmp(s, t, DIRSIZ);
5956 }
5957 
5958 // Look for a directory entry in a directory.
5959 // If found, set *poff to byte offset of entry.
5960 struct inode*
5961 dirlookup(struct inode *dp, char *name, uint *poff)
5962 {
5963   uint off, inum;
5964   struct dirent de;
5965 
5966   if(dp->type != T_DIR)
5967     panic("dirlookup not DIR");
5968 
5969   for(off = 0; off < dp->size; off += sizeof(de)){
5970     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5971       panic("dirlookup read");
5972     if(de.inum == 0)
5973       continue;
5974     if(namecmp(name, de.name) == 0){
5975       // entry matches path element
5976       if(poff)
5977         *poff = off;
5978       inum = de.inum;
5979       return iget(dp->dev, inum);
5980     }
5981   }
5982 
5983   return 0;
5984 }
5985 
5986 
5987 
5988 
5989 
5990 
5991 
5992 
5993 
5994 
5995 
5996 
5997 
5998 
5999 
6000 // Write a new directory entry (name, inum) into the directory dp.
6001 int
6002 dirlink(struct inode *dp, char *name, uint inum)
6003 {
6004   int off;
6005   struct dirent de;
6006   struct inode *ip;
6007 
6008   // Check that name is not present.
6009   if((ip = dirlookup(dp, name, 0)) != 0){
6010     iput(ip);
6011     return -1;
6012   }
6013 
6014   // Look for an empty dirent.
6015   for(off = 0; off < dp->size; off += sizeof(de)){
6016     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6017       panic("dirlink read");
6018     if(de.inum == 0)
6019       break;
6020   }
6021 
6022   strncpy(de.name, name, DIRSIZ);
6023   de.inum = inum;
6024   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6025     panic("dirlink");
6026 
6027   return 0;
6028 }
6029 
6030 
6031 
6032 
6033 
6034 
6035 
6036 
6037 
6038 
6039 
6040 
6041 
6042 
6043 
6044 
6045 
6046 
6047 
6048 
6049 
6050 // Paths
6051 
6052 // Copy the next path element from path into name.
6053 // Return a pointer to the element following the copied one.
6054 // The returned path has no leading slashes,
6055 // so the caller can check *path=='\0' to see if the name is the last one.
6056 // If no name to remove, return 0.
6057 //
6058 // Examples:
6059 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
6060 //   skipelem("///a//bb", name) = "bb", setting name = "a"
6061 //   skipelem("a", name) = "", setting name = "a"
6062 //   skipelem("", name) = skipelem("////", name) = 0
6063 //
6064 static char*
6065 skipelem(char *path, char *name)
6066 {
6067   char *s;
6068   int len;
6069 
6070   while(*path == '/')
6071     path++;
6072   if(*path == 0)
6073     return 0;
6074   s = path;
6075   while(*path != '/' && *path != 0)
6076     path++;
6077   len = path - s;
6078   if(len >= DIRSIZ)
6079     memmove(name, s, DIRSIZ);
6080   else {
6081     memmove(name, s, len);
6082     name[len] = 0;
6083   }
6084   while(*path == '/')
6085     path++;
6086   return path;
6087 }
6088 
6089 
6090 
6091 
6092 
6093 
6094 
6095 
6096 
6097 
6098 
6099 
6100 // Look up and return the inode for a path name.
6101 // If parent != 0, return the inode for the parent and copy the final
6102 // path element into name, which must have room for DIRSIZ bytes.
6103 // Must be called inside a transaction since it calls iput().
6104 static struct inode*
6105 namex(char *path, int nameiparent, char *name)
6106 {
6107   struct inode *ip, *next;
6108 
6109   if(*path == '/')
6110     ip = iget(ROOTDEV, ROOTINO);
6111   else
6112     ip = idup(myproc()->cwd);
6113 
6114   while((path = skipelem(path, name)) != 0){
6115     ilock(ip);
6116     if(ip->type != T_DIR){
6117       iunlockput(ip);
6118       return 0;
6119     }
6120     if(nameiparent && *path == '\0'){
6121       // Stop one level early.
6122       iunlock(ip);
6123       return ip;
6124     }
6125     if((next = dirlookup(ip, name, 0)) == 0){
6126       iunlockput(ip);
6127       return 0;
6128     }
6129     iunlockput(ip);
6130     ip = next;
6131   }
6132   if(nameiparent){
6133     iput(ip);
6134     return 0;
6135   }
6136   return ip;
6137 }
6138 
6139 struct inode*
6140 namei(char *path)
6141 {
6142   char name[DIRSIZ];
6143   return namex(path, 0, name);
6144 }
6145 
6146 
6147 
6148 
6149 
6150 struct inode*
6151 nameiparent(char *path, char *name)
6152 {
6153   return namex(path, 1, name);
6154 }
6155 
6156 
6157 
6158 
6159 
6160 
6161 
6162 
6163 
6164 
6165 
6166 
6167 
6168 
6169 
6170 
6171 
6172 
6173 
6174 
6175 
6176 
6177 
6178 
6179 
6180 
6181 
6182 
6183 
6184 
6185 
6186 
6187 
6188 
6189 
6190 
6191 
6192 
6193 
6194 
6195 
6196 
6197 
6198 
6199 
