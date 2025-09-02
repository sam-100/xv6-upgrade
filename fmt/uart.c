8650 // Intel 8250 serial port (UART).
8651 
8652 #include "types.h"
8653 #include "defs.h"
8654 #include "param.h"
8655 #include "traps.h"
8656 #include "spinlock.h"
8657 #include "sleeplock.h"
8658 #include "fs.h"
8659 #include "file.h"
8660 #include "mmu.h"
8661 #include "proc.h"
8662 #include "x86.h"
8663 
8664 #define COM1    0x3f8
8665 
8666 static int uart;    // is there a uart?
8667 
8668 void
8669 uartinit(void)
8670 {
8671   char *p;
8672 
8673   // Turn off the FIFO
8674   outb(COM1+2, 0);
8675 
8676   // 9600 baud, 8 data bits, 1 stop bit, parity off.
8677   outb(COM1+3, 0x80);    // Unlock divisor
8678   outb(COM1+0, 115200/9600);
8679   outb(COM1+1, 0);
8680   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
8681   outb(COM1+4, 0);
8682   outb(COM1+1, 0x01);    // Enable receive interrupts.
8683 
8684   // If status is 0xFF, no serial port.
8685   if(inb(COM1+5) == 0xFF)
8686     return;
8687   uart = 1;
8688 
8689   // Acknowledge pre-existing interrupt conditions;
8690   // enable interrupts.
8691   inb(COM1+2);
8692   inb(COM1+0);
8693   ioapicenable(IRQ_COM1, 0);
8694 
8695   // Announce that we're here.
8696   for(p="xv6...\n"; *p; p++)
8697     uartputc(*p);
8698 }
8699 
8700 void
8701 uartputc(int c)
8702 {
8703   int i;
8704 
8705   if(!uart)
8706     return;
8707   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
8708     microdelay(10);
8709   outb(COM1+0, c);
8710 }
8711 
8712 static int
8713 uartgetc(void)
8714 {
8715   if(!uart)
8716     return -1;
8717   if(!(inb(COM1+5) & 0x01))
8718     return -1;
8719   return inb(COM1+0);
8720 }
8721 
8722 void
8723 uartintr(void)
8724 {
8725   consoleintr(uartgetc);
8726 }
8727 
8728 
8729 
8730 
8731 
8732 
8733 
8734 
8735 
8736 
8737 
8738 
8739 
8740 
8741 
8742 
8743 
8744 
8745 
8746 
8747 
8748 
8749 
