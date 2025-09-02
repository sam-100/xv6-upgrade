7950 // The I/O APIC manages hardware interrupts for an SMP system.
7951 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
7952 // See also picirq.c.
7953 
7954 #include "types.h"
7955 #include "defs.h"
7956 #include "traps.h"
7957 
7958 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
7959 
7960 #define REG_ID     0x00  // Register index: ID
7961 #define REG_VER    0x01  // Register index: version
7962 #define REG_TABLE  0x10  // Redirection table base
7963 
7964 // The redirection table starts at REG_TABLE and uses
7965 // two registers to configure each interrupt.
7966 // The first (low) register in a pair contains configuration bits.
7967 // The second (high) register contains a bitmask telling which
7968 // CPUs can serve that interrupt.
7969 #define INT_DISABLED   0x00010000  // Interrupt disabled
7970 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
7971 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
7972 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
7973 
7974 volatile struct ioapic *ioapic;
7975 
7976 // IO APIC MMIO structure: write reg, then read or write data.
7977 struct ioapic {
7978   uint reg;
7979   uint pad[3];
7980   uint data;
7981 };
7982 
7983 static uint
7984 ioapicread(int reg)
7985 {
7986   ioapic->reg = reg;
7987   return ioapic->data;
7988 }
7989 
7990 static void
7991 ioapicwrite(int reg, uint data)
7992 {
7993   ioapic->reg = reg;
7994   ioapic->data = data;
7995 }
7996 
7997 
7998 
7999 
8000 void
8001 ioapicinit(void)
8002 {
8003   int i, id, maxintr;
8004 
8005   ioapic = (volatile struct ioapic*)IOAPIC;
8006   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
8007   id = ioapicread(REG_ID) >> 24;
8008   if(id != ioapicid)
8009     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
8010 
8011   // Mark all interrupts edge-triggered, active high, disabled,
8012   // and not routed to any CPUs.
8013   for(i = 0; i <= maxintr; i++){
8014     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
8015     ioapicwrite(REG_TABLE+2*i+1, 0);
8016   }
8017 }
8018 
8019 void
8020 ioapicenable(int irq, int cpunum)
8021 {
8022   // Mark interrupt edge-triggered, active high,
8023   // enabled, and routed to the given cpunum,
8024   // which happens to be that cpu's APIC ID.
8025   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
8026   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
8027 }
8028 
8029 
8030 
8031 
8032 
8033 
8034 
8035 
8036 
8037 
8038 
8039 
8040 
8041 
8042 
8043 
8044 
8045 
8046 
8047 
8048 
8049 
