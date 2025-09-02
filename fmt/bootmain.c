9550 // Boot loader.
9551 //
9552 // Part of the boot block, along with bootasm.S, which calls bootmain().
9553 // bootasm.S has put the processor into protected 32-bit mode.
9554 // bootmain() loads an ELF kernel image from the disk starting at
9555 // sector 1 and then jumps to the kernel entry routine.
9556 
9557 #include "types.h"
9558 #include "elf.h"
9559 #include "x86.h"
9560 #include "memlayout.h"
9561 
9562 #define SECTSIZE  512
9563 
9564 void readseg(uchar*, uint, uint);
9565 
9566 void
9567 bootmain(void)
9568 {
9569   struct elfhdr *elf;
9570   struct proghdr *ph, *eph;
9571   void (*entry)(void);
9572   uchar* pa;
9573 
9574   elf = (struct elfhdr*)0x10000;  // scratch space
9575 
9576   // Read 1st page off disk
9577   readseg((uchar*)elf, 4096, 0);
9578 
9579   // Is this an ELF executable?
9580   if(elf->magic != ELF_MAGIC)
9581     return;  // let bootasm.S handle error
9582 
9583   // Load each program segment (ignores ph flags).
9584   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
9585   eph = ph + elf->phnum;
9586   for(; ph < eph; ph++){
9587     pa = (uchar*)ph->paddr;
9588     readseg(pa, ph->filesz, ph->off);
9589     if(ph->memsz > ph->filesz)
9590       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
9591   }
9592 
9593   // Call the entry point from the ELF header.
9594   // Does not return!
9595   entry = (void(*)(void))(elf->entry);
9596   entry();
9597 }
9598 
9599 
9600 void
9601 waitdisk(void)
9602 {
9603   // Wait for disk ready.
9604   while((inb(0x1F7) & 0xC0) != 0x40)
9605     ;
9606 }
9607 
9608 // Read a single sector at offset into dst.
9609 void
9610 readsect(void *dst, uint offset)
9611 {
9612   // Issue command.
9613   waitdisk();
9614   outb(0x1F2, 1);   // count = 1
9615   outb(0x1F3, offset);
9616   outb(0x1F4, offset >> 8);
9617   outb(0x1F5, offset >> 16);
9618   outb(0x1F6, (offset >> 24) | 0xE0);
9619   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
9620 
9621   // Read data.
9622   waitdisk();
9623   insl(0x1F0, dst, SECTSIZE/4);
9624 }
9625 
9626 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
9627 // Might copy more than asked.
9628 void
9629 readseg(uchar* pa, uint count, uint offset)
9630 {
9631   uchar* epa;
9632 
9633   epa = pa + count;
9634 
9635   // Round down to sector boundary.
9636   pa -= offset % SECTSIZE;
9637 
9638   // Translate from bytes to sectors; kernel starts at sector 1.
9639   offset = (offset / SECTSIZE) + 1;
9640 
9641   // If this is too slow, we could read lots of sectors at a time.
9642   // We'd write more to memory than asked, but it doesn't matter --
9643   // we load in increasing order.
9644   for(; pa < epa; pa += SECTSIZE, offset++)
9645     readsect(pa, offset);
9646 }
9647 
9648 
9649 
