7700 // The local APIC manages internal (non-I/O) interrupts.
7701 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
7702 
7703 #include "param.h"
7704 #include "types.h"
7705 #include "defs.h"
7706 #include "date.h"
7707 #include "memlayout.h"
7708 #include "traps.h"
7709 #include "mmu.h"
7710 #include "x86.h"
7711 
7712 // Local APIC registers, divided by 4 for use as uint[] indices.
7713 #define ID      (0x0020/4)   // ID
7714 #define VER     (0x0030/4)   // Version
7715 #define TPR     (0x0080/4)   // Task Priority
7716 #define EOI     (0x00B0/4)   // EOI
7717 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
7718   #define ENABLE     0x00000100   // Unit Enable
7719 #define ESR     (0x0280/4)   // Error Status
7720 #define ICRLO   (0x0300/4)   // Interrupt Command
7721   #define INIT       0x00000500   // INIT/RESET
7722   #define STARTUP    0x00000600   // Startup IPI
7723   #define DELIVS     0x00001000   // Delivery status
7724   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
7725   #define DEASSERT   0x00000000
7726   #define LEVEL      0x00008000   // Level triggered
7727   #define BCAST      0x00080000   // Send to all APICs, including self.
7728   #define BUSY       0x00001000
7729   #define FIXED      0x00000000
7730 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
7731 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
7732   #define X1         0x0000000B   // divide counts by 1
7733   #define PERIODIC   0x00020000   // Periodic
7734 #define PCINT   (0x0340/4)   // Performance Counter LVT
7735 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
7736 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
7737 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
7738   #define MASKED     0x00010000   // Interrupt masked
7739 #define TICR    (0x0380/4)   // Timer Initial Count
7740 #define TCCR    (0x0390/4)   // Timer Current Count
7741 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
7742 
7743 volatile uint *lapic;  // Initialized in mp.c
7744 
7745 
7746 
7747 
7748 
7749 
7750 static void
7751 lapicw(int index, int value)
7752 {
7753   lapic[index] = value;
7754   lapic[ID];  // wait for write to finish, by reading
7755 }
7756 
7757 void
7758 lapicinit(void)
7759 {
7760   if(!lapic)
7761     return;
7762 
7763   // Enable local APIC; set spurious interrupt vector.
7764   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
7765 
7766   // The timer repeatedly counts down at bus frequency
7767   // from lapic[TICR] and then issues an interrupt.
7768   // If xv6 cared more about precise timekeeping,
7769   // TICR would be calibrated using an external time source.
7770   lapicw(TDCR, X1);
7771   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
7772   lapicw(TICR, 10000000);
7773 
7774   // Disable logical interrupt lines.
7775   lapicw(LINT0, MASKED);
7776   lapicw(LINT1, MASKED);
7777 
7778   // Disable performance counter overflow interrupts
7779   // on machines that provide that interrupt entry.
7780   if(((lapic[VER]>>16) & 0xFF) >= 4)
7781     lapicw(PCINT, MASKED);
7782 
7783   // Map error interrupt to IRQ_ERROR.
7784   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
7785 
7786   // Clear error status register (requires back-to-back writes).
7787   lapicw(ESR, 0);
7788   lapicw(ESR, 0);
7789 
7790   // Ack any outstanding interrupts.
7791   lapicw(EOI, 0);
7792 
7793   // Send an Init Level De-Assert to synchronise arbitration ID's.
7794   lapicw(ICRHI, 0);
7795   lapicw(ICRLO, BCAST | INIT | LEVEL);
7796   while(lapic[ICRLO] & DELIVS)
7797     ;
7798 
7799 
7800   // Enable interrupts on the APIC (but not on the processor).
7801   lapicw(TPR, 0);
7802 }
7803 
7804 int
7805 lapicid(void)
7806 {
7807   if (!lapic)
7808     return 0;
7809   return lapic[ID] >> 24;
7810 }
7811 
7812 // Acknowledge interrupt.
7813 void
7814 lapiceoi(void)
7815 {
7816   if(lapic)
7817     lapicw(EOI, 0);
7818 }
7819 
7820 // Spin for a given number of microseconds.
7821 // On real hardware would want to tune this dynamically.
7822 void
7823 microdelay(int us)
7824 {
7825 }
7826 
7827 #define CMOS_PORT    0x70
7828 #define CMOS_RETURN  0x71
7829 
7830 // Start additional processor running entry code at addr.
7831 // See Appendix B of MultiProcessor Specification.
7832 void
7833 lapicstartap(uchar apicid, uint addr)
7834 {
7835   int i;
7836   ushort *wrv;
7837 
7838   // "The BSP must initialize CMOS shutdown code to 0AH
7839   // and the warm reset vector (DWORD based at 40:67) to point at
7840   // the AP startup code prior to the [universal startup algorithm]."
7841   outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
7842   outb(CMOS_PORT+1, 0x0A);
7843   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
7844   wrv[0] = 0;
7845   wrv[1] = addr >> 4;
7846 
7847 
7848 
7849 
7850   // "Universal startup algorithm."
7851   // Send INIT (level-triggered) interrupt to reset other CPU.
7852   lapicw(ICRHI, apicid<<24);
7853   lapicw(ICRLO, INIT | LEVEL | ASSERT);
7854   microdelay(200);
7855   lapicw(ICRLO, INIT | LEVEL);
7856   microdelay(100);    // should be 10ms, but too slow in Bochs!
7857 
7858   // Send startup IPI (twice!) to enter code.
7859   // Regular hardware is supposed to only accept a STARTUP
7860   // when it is in the halted state due to an INIT.  So the second
7861   // should be ignored, but it is part of the official Intel algorithm.
7862   // Bochs complains about the second one.  Too bad for Bochs.
7863   for(i = 0; i < 2; i++){
7864     lapicw(ICRHI, apicid<<24);
7865     lapicw(ICRLO, STARTUP | (addr>>12));
7866     microdelay(200);
7867   }
7868 }
7869 
7870 #define CMOS_STATA   0x0a
7871 #define CMOS_STATB   0x0b
7872 #define CMOS_UIP    (1 << 7)        // RTC update in progress
7873 
7874 #define SECS    0x00
7875 #define MINS    0x02
7876 #define HOURS   0x04
7877 #define DAY     0x07
7878 #define MONTH   0x08
7879 #define YEAR    0x09
7880 
7881 static uint
7882 cmos_read(uint reg)
7883 {
7884   outb(CMOS_PORT,  reg);
7885   microdelay(200);
7886 
7887   return inb(CMOS_RETURN);
7888 }
7889 
7890 static void
7891 fill_rtcdate(struct rtcdate *r)
7892 {
7893   r->second = cmos_read(SECS);
7894   r->minute = cmos_read(MINS);
7895   r->hour   = cmos_read(HOURS);
7896   r->day    = cmos_read(DAY);
7897   r->month  = cmos_read(MONTH);
7898   r->year   = cmos_read(YEAR);
7899 }
7900 // qemu seems to use 24-hour GWT and the values are BCD encoded
7901 void
7902 cmostime(struct rtcdate *r)
7903 {
7904   struct rtcdate t1, t2;
7905   int sb, bcd;
7906 
7907   sb = cmos_read(CMOS_STATB);
7908 
7909   bcd = (sb & (1 << 2)) == 0;
7910 
7911   // make sure CMOS doesn't modify time while we read it
7912   for(;;) {
7913     fill_rtcdate(&t1);
7914     if(cmos_read(CMOS_STATA) & CMOS_UIP)
7915         continue;
7916     fill_rtcdate(&t2);
7917     if(memcmp(&t1, &t2, sizeof(t1)) == 0)
7918       break;
7919   }
7920 
7921   // convert
7922   if(bcd) {
7923 #define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
7924     CONV(second);
7925     CONV(minute);
7926     CONV(hour  );
7927     CONV(day   );
7928     CONV(month );
7929     CONV(year  );
7930 #undef     CONV
7931   }
7932 
7933   *r = t1;
7934   r->year += 2000;
7935 }
7936 
7937 
7938 
7939 
7940 
7941 
7942 
7943 
7944 
7945 
7946 
7947 
7948 
7949 
