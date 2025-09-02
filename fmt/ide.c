4550 // Simple PIO-based (non-DMA) IDE driver code.
4551 
4552 #include "types.h"
4553 #include "defs.h"
4554 #include "param.h"
4555 #include "memlayout.h"
4556 #include "mmu.h"
4557 #include "proc.h"
4558 #include "x86.h"
4559 #include "traps.h"
4560 #include "spinlock.h"
4561 #include "sleeplock.h"
4562 #include "fs.h"
4563 #include "buf.h"
4564 
4565 #define SECTOR_SIZE   512
4566 #define IDE_BSY       0x80
4567 #define IDE_DRDY      0x40
4568 #define IDE_DF        0x20
4569 #define IDE_ERR       0x01
4570 
4571 #define IDE_CMD_READ  0x20
4572 #define IDE_CMD_WRITE 0x30
4573 #define IDE_CMD_RDMUL 0xc4
4574 #define IDE_CMD_WRMUL 0xc5
4575 
4576 // idequeue points to the buf now being read/written to the disk.
4577 // idequeue->qnext points to the next buf to be processed.
4578 // You must hold idelock while manipulating queue.
4579 
4580 static struct spinlock idelock;
4581 static struct buf *idequeue;
4582 
4583 static int havedisk1;
4584 static void idestart(struct buf*);
4585 
4586 // Wait for IDE disk to become ready.
4587 static int
4588 idewait(int checkerr)
4589 {
4590   int r;
4591 
4592   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
4593     ;
4594   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
4595     return -1;
4596   return 0;
4597 }
4598 
4599 
4600 void
4601 ideinit(void)
4602 {
4603   int i;
4604 
4605   initlock(&idelock, "ide");
4606   ioapicenable(IRQ_IDE, ncpu - 1);
4607   idewait(0);
4608 
4609   // Check if disk 1 is present
4610   outb(0x1f6, 0xe0 | (1<<4));
4611   for(i=0; i<1000; i++){
4612     if(inb(0x1f7) != 0){
4613       havedisk1 = 1;
4614       break;
4615     }
4616   }
4617 
4618   // Switch back to disk 0.
4619   outb(0x1f6, 0xe0 | (0<<4));
4620 }
4621 
4622 // Start the request for b.  Caller must hold idelock.
4623 static void
4624 idestart(struct buf *b)
4625 {
4626   if(b == 0)
4627     panic("idestart");
4628   if(b->blockno >= FSSIZE)
4629     panic("incorrect blockno");
4630   int sector_per_block =  BSIZE/SECTOR_SIZE;
4631   int sector = b->blockno * sector_per_block;
4632   int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
4633   int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
4634 
4635   if (sector_per_block > 7) panic("idestart");
4636 
4637   idewait(0);
4638   outb(0x3f6, 0);  // generate interrupt
4639   outb(0x1f2, sector_per_block);  // number of sectors
4640   outb(0x1f3, sector & 0xff);
4641   outb(0x1f4, (sector >> 8) & 0xff);
4642   outb(0x1f5, (sector >> 16) & 0xff);
4643   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
4644   if(b->flags & B_DIRTY){
4645     outb(0x1f7, write_cmd);
4646     outsl(0x1f0, b->data, BSIZE/4);
4647   } else {
4648     outb(0x1f7, read_cmd);
4649   }
4650 }
4651 
4652 // Interrupt handler.
4653 void
4654 ideintr(void)
4655 {
4656   struct buf *b;
4657 
4658   // First queued buffer is the active request.
4659   acquire(&idelock);
4660 
4661   if((b = idequeue) == 0){
4662     release(&idelock);
4663     return;
4664   }
4665   idequeue = b->qnext;
4666 
4667   // Read data if needed.
4668   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4669     insl(0x1f0, b->data, BSIZE/4);
4670 
4671   // Wake process waiting for this buf.
4672   b->flags |= B_VALID;
4673   b->flags &= ~B_DIRTY;
4674   wakeup(b);
4675 
4676   // Start disk on next buf in queue.
4677   if(idequeue != 0)
4678     idestart(idequeue);
4679 
4680   release(&idelock);
4681 }
4682 
4683 
4684 
4685 
4686 
4687 
4688 
4689 
4690 
4691 
4692 
4693 
4694 
4695 
4696 
4697 
4698 
4699 
4700 // Sync buf with disk.
4701 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4702 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4703 void
4704 iderw(struct buf *b)
4705 {
4706   struct buf **pp;
4707 
4708   if(!holdingsleep(&b->lock))
4709     panic("iderw: buf not locked");
4710   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4711     panic("iderw: nothing to do");
4712   if(b->dev != 0 && !havedisk1)
4713     panic("iderw: ide disk 1 not present");
4714 
4715   acquire(&idelock);  //DOC:acquire-lock
4716 
4717   // Append b to idequeue.
4718   b->qnext = 0;
4719   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4720     ;
4721   *pp = b;
4722 
4723   // Start disk if necessary.
4724   if(idequeue == b)
4725     idestart(b);
4726 
4727   // Wait for request to finish.
4728   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4729     sleep(b, &idelock);
4730   }
4731 
4732 
4733   release(&idelock);
4734 }
4735 
4736 
4737 
4738 
4739 
4740 
4741 
4742 
4743 
4744 
4745 
4746 
4747 
4748 
4749 
