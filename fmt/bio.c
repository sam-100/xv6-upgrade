4750 // Buffer cache.
4751 //
4752 // The buffer cache is a linked list of buf structures holding
4753 // cached copies of disk block contents.  Caching disk blocks
4754 // in memory reduces the number of disk reads and also provides
4755 // a synchronization point for disk blocks used by multiple processes.
4756 //
4757 // Interface:
4758 // * To get a buffer for a particular disk block, call bread.
4759 // * After changing buffer data, call bwrite to write it to disk.
4760 // * When done with the buffer, call brelse.
4761 // * Do not use the buffer after calling brelse.
4762 // * Only one process at a time can use a buffer,
4763 //     so do not keep them longer than necessary.
4764 //
4765 // The implementation uses two state flags internally:
4766 // * B_VALID: the buffer data has been read from the disk.
4767 // * B_DIRTY: the buffer data has been modified
4768 //     and needs to be written to disk.
4769 
4770 #include "types.h"
4771 #include "defs.h"
4772 #include "param.h"
4773 #include "spinlock.h"
4774 #include "sleeplock.h"
4775 #include "fs.h"
4776 #include "buf.h"
4777 
4778 struct {
4779   struct spinlock lock;
4780   struct buf buf[NBUF];
4781 
4782   // Linked list of all buffers, through prev/next.
4783   // head.next is most recently used.
4784   struct buf head;
4785 } bcache;
4786 
4787 void
4788 binit(void)
4789 {
4790   struct buf *b;
4791 
4792   initlock(&bcache.lock, "bcache");
4793 
4794 
4795 
4796 
4797 
4798 
4799 
4800   // Create linked list of buffers
4801   bcache.head.prev = &bcache.head;
4802   bcache.head.next = &bcache.head;
4803   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4804     b->next = bcache.head.next;
4805     b->prev = &bcache.head;
4806     initsleeplock(&b->lock, "buffer");
4807     bcache.head.next->prev = b;
4808     bcache.head.next = b;
4809   }
4810 }
4811 
4812 // Look through buffer cache for block on device dev.
4813 // If not found, allocate a buffer.
4814 // In either case, return locked buffer.
4815 static struct buf*
4816 bget(uint dev, uint blockno)
4817 {
4818   struct buf *b;
4819 
4820   acquire(&bcache.lock);
4821 
4822   // Is the block already cached?
4823   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4824     if(b->dev == dev && b->blockno == blockno){
4825       b->refcnt++;
4826       release(&bcache.lock);
4827       acquiresleep(&b->lock);
4828       return b;
4829     }
4830   }
4831 
4832   // Not cached; recycle an unused buffer.
4833   // Even if refcnt==0, B_DIRTY indicates a buffer is in use
4834   // because log.c has modified it but not yet committed it.
4835   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4836     if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
4837       b->dev = dev;
4838       b->blockno = blockno;
4839       b->flags = 0;
4840       b->refcnt = 1;
4841       release(&bcache.lock);
4842       acquiresleep(&b->lock);
4843       return b;
4844     }
4845   }
4846   panic("bget: no buffers");
4847 }
4848 
4849 
4850 // Return a locked buf with the contents of the indicated block.
4851 struct buf*
4852 bread(uint dev, uint blockno)
4853 {
4854   struct buf *b;
4855 
4856   b = bget(dev, blockno);
4857   if((b->flags & B_VALID) == 0) {
4858     iderw(b);
4859   }
4860   return b;
4861 }
4862 
4863 // Write b's contents to disk.  Must be locked.
4864 void
4865 bwrite(struct buf *b)
4866 {
4867   if(!holdingsleep(&b->lock))
4868     panic("bwrite");
4869   b->flags |= B_DIRTY;
4870   iderw(b);
4871 }
4872 
4873 // Release a locked buffer.
4874 // Move to the head of the MRU list.
4875 void
4876 brelse(struct buf *b)
4877 {
4878   if(!holdingsleep(&b->lock))
4879     panic("brelse");
4880 
4881   releasesleep(&b->lock);
4882 
4883   acquire(&bcache.lock);
4884   b->refcnt--;
4885   if (b->refcnt == 0) {
4886     // no one is waiting for it.
4887     b->next->prev = b->prev;
4888     b->prev->next = b->next;
4889     b->next = bcache.head.next;
4890     b->prev = &bcache.head;
4891     bcache.head.next->prev = b;
4892     bcache.head.next = b;
4893   }
4894 
4895   release(&bcache.lock);
4896 }
4897 
4898 
4899 
4900 // Blank page.
4901 
4902 
4903 
4904 
4905 
4906 
4907 
4908 
4909 
4910 
4911 
4912 
4913 
4914 
4915 
4916 
4917 
4918 
4919 
4920 
4921 
4922 
4923 
4924 
4925 
4926 
4927 
4928 
4929 
4930 
4931 
4932 
4933 
4934 
4935 
4936 
4937 
4938 
4939 
4940 
4941 
4942 
4943 
4944 
4945 
4946 
4947 
4948 
4949 
