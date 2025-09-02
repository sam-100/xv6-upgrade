9450 #include "asm.h"
9451 #include "memlayout.h"
9452 #include "mmu.h"
9453 
9454 # Start the first CPU: switch to 32-bit protected mode, jump into C.
9455 # The BIOS loads this code from the first sector of the hard disk into
9456 # memory at physical address 0x7c00 and starts executing in real mode
9457 # with %cs=0 %ip=7c00.
9458 
9459 .code16                       # Assemble for 16-bit mode
9460 .globl start
9461 start:
9462   cli                         # BIOS enabled interrupts; disable
9463 
9464   # Zero data segment registers DS, ES, and SS.
9465   xorw    %ax,%ax             # Set %ax to zero
9466   movw    %ax,%ds             # -> Data Segment
9467   movw    %ax,%es             # -> Extra Segment
9468   movw    %ax,%ss             # -> Stack Segment
9469 
9470   # Physical address line A20 is tied to zero so that the first PCs
9471   # with 2 MB would run software that assumed 1 MB.  Undo that.
9472 seta20.1:
9473   inb     $0x64,%al               # Wait for not busy
9474   testb   $0x2,%al
9475   jnz     seta20.1
9476 
9477   movb    $0xd1,%al               # 0xd1 -> port 0x64
9478   outb    %al,$0x64
9479 
9480 seta20.2:
9481   inb     $0x64,%al               # Wait for not busy
9482   testb   $0x2,%al
9483   jnz     seta20.2
9484 
9485   movb    $0xdf,%al               # 0xdf -> port 0x60
9486   outb    %al,$0x60
9487 
9488   # Switch from real to protected mode.  Use a bootstrap GDT that makes
9489   # virtual addresses map directly to physical addresses so that the
9490   # effective memory map doesn't change during the transition.
9491   lgdt    gdtdesc
9492   movl    %cr0, %eax
9493   orl     $CR0_PE, %eax
9494   movl    %eax, %cr0
9495 
9496 
9497 
9498 
9499 
9500   # Complete the transition to 32-bit protected mode by using a long jmp
9501   # to reload %cs and %eip.  The segment descriptors are set up with no
9502   # translation, so that the mapping is still the identity mapping.
9503   ljmp    $(SEG_KCODE<<3), $start32
9504 
9505 .code32  # Tell assembler to generate 32-bit code now.
9506 start32:
9507   # Set up the protected-mode data segment registers
9508   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
9509   movw    %ax, %ds                # -> DS: Data Segment
9510   movw    %ax, %es                # -> ES: Extra Segment
9511   movw    %ax, %ss                # -> SS: Stack Segment
9512   movw    $0, %ax                 # Zero segments not ready for use
9513   movw    %ax, %fs                # -> FS
9514   movw    %ax, %gs                # -> GS
9515 
9516   # Set up the stack pointer and call into C.
9517   movl    $start, %esp
9518   call    bootmain
9519 
9520   # If bootmain returns (it shouldn't), trigger a Bochs
9521   # breakpoint if running under Bochs, then loop.
9522   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
9523   movw    %ax, %dx
9524   outw    %ax, %dx
9525   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
9526   outw    %ax, %dx
9527 spin:
9528   jmp     spin
9529 
9530 # Bootstrap GDT
9531 .p2align 2                                # force 4 byte alignment
9532 gdt:
9533   SEG_NULLASM                             # null seg
9534   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
9535   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
9536 
9537 gdtdesc:
9538   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
9539   .long   gdt                             # address gdt
9540 
9541 
9542 
9543 
9544 
9545 
9546 
9547 
9548 
9549 
