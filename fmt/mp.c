7550 // Multiprocessor support
7551 // Search memory for MP description structures.
7552 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
7553 
7554 #include "types.h"
7555 #include "defs.h"
7556 #include "param.h"
7557 #include "memlayout.h"
7558 #include "mp.h"
7559 #include "x86.h"
7560 #include "mmu.h"
7561 #include "proc.h"
7562 
7563 struct cpu cpus[NCPU];
7564 int ncpu;
7565 uchar ioapicid;
7566 
7567 static uchar
7568 sum(uchar *addr, int len)
7569 {
7570   int i, sum;
7571 
7572   sum = 0;
7573   for(i=0; i<len; i++)
7574     sum += addr[i];
7575   return sum;
7576 }
7577 
7578 // Look for an MP structure in the len bytes at addr.
7579 static struct mp*
7580 mpsearch1(uint a, int len)
7581 {
7582   uchar *e, *p, *addr;
7583 
7584   addr = P2V(a);
7585   e = addr+len;
7586   for(p = addr; p < e; p += sizeof(struct mp))
7587     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
7588       return (struct mp*)p;
7589   return 0;
7590 }
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
7600 // Search for the MP Floating Pointer Structure, which according to the
7601 // spec is in one of the following three locations:
7602 // 1) in the first KB of the EBDA;
7603 // 2) in the last KB of system base memory;
7604 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
7605 static struct mp*
7606 mpsearch(void)
7607 {
7608   uchar *bda;
7609   uint p;
7610   struct mp *mp;
7611 
7612   bda = (uchar *) P2V(0x400);
7613   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
7614     if((mp = mpsearch1(p, 1024)))
7615       return mp;
7616   } else {
7617     p = ((bda[0x14]<<8)|bda[0x13])*1024;
7618     if((mp = mpsearch1(p-1024, 1024)))
7619       return mp;
7620   }
7621   return mpsearch1(0xF0000, 0x10000);
7622 }
7623 
7624 // Search for an MP configuration table.  For now,
7625 // don't accept the default configurations (physaddr == 0).
7626 // Check for correct signature, calculate the checksum and,
7627 // if correct, check the version.
7628 // To do: check extended table checksum.
7629 static struct mpconf*
7630 mpconfig(struct mp **pmp)
7631 {
7632   struct mpconf *conf;
7633   struct mp *mp;
7634 
7635   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
7636     return 0;
7637   conf = (struct mpconf*) P2V((uint) mp->physaddr);
7638   if(memcmp(conf, "PCMP", 4) != 0)
7639     return 0;
7640   if(conf->version != 1 && conf->version != 4)
7641     return 0;
7642   if(sum((uchar*)conf, conf->length) != 0)
7643     return 0;
7644   *pmp = mp;
7645   return conf;
7646 }
7647 
7648 
7649 
7650 void
7651 mpinit(void)
7652 {
7653   uchar *p, *e;
7654   int ismp;
7655   struct mp *mp;
7656   struct mpconf *conf;
7657   struct mpproc *proc;
7658   struct mpioapic *ioapic;
7659 
7660   if((conf = mpconfig(&mp)) == 0)
7661     panic("Expect to run on an SMP");
7662   ismp = 1;
7663   lapic = (uint*)conf->lapicaddr;
7664   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
7665     switch(*p){
7666     case MPPROC:
7667       proc = (struct mpproc*)p;
7668       if(ncpu < NCPU) {
7669         cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
7670         ncpu++;
7671       }
7672       p += sizeof(struct mpproc);
7673       continue;
7674     case MPIOAPIC:
7675       ioapic = (struct mpioapic*)p;
7676       ioapicid = ioapic->apicno;
7677       p += sizeof(struct mpioapic);
7678       continue;
7679     case MPBUS:
7680     case MPIOINTR:
7681     case MPLINTR:
7682       p += 8;
7683       continue;
7684     default:
7685       ismp = 0;
7686       break;
7687     }
7688   }
7689   if(!ismp)
7690     panic("Didn't find a suitable machine");
7691 
7692   if(mp->imcrp){
7693     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
7694     // But it would on real hardware.
7695     outb(0x22, 0x70);   // Select IMCR
7696     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
7697   }
7698 }
7699 
