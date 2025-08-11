0750 // This file contains definitions for the
0751 // x86 memory management unit (MMU).
0752 
0753 // Eflags register
0754 #define FL_IF           0x00000200      // Interrupt Enable
0755 
0756 // Control Register flags
0757 #define CR0_PE          0x00000001      // Protection Enable
0758 #define CR0_WP          0x00010000      // Write Protect
0759 #define CR0_PG          0x80000000      // Paging
0760 
0761 #define CR4_PSE         0x00000010      // Page size extension
0762 
0763 // various segment selectors.
0764 #define SEG_KCODE 1  // kernel code
0765 #define SEG_KDATA 2  // kernel data+stack
0766 #define SEG_UCODE 3  // user code
0767 #define SEG_UDATA 4  // user data+stack
0768 #define SEG_TSS   5  // this process's task state
0769 
0770 // cpu->gdt[NSEGS] holds the above segments.
0771 #define NSEGS     6
0772 
0773 #ifndef __ASSEMBLER__
0774 // Segment Descriptor
0775 struct segdesc {
0776   uint lim_15_0 : 16;  // Low bits of segment limit
0777   uint base_15_0 : 16; // Low bits of segment base address
0778   uint base_23_16 : 8; // Middle bits of segment base address
0779   uint type : 4;       // Segment type (see STS_ constants)
0780   uint s : 1;          // 0 = system, 1 = application
0781   uint dpl : 2;        // Descriptor Privilege Level
0782   uint p : 1;          // Present
0783   uint lim_19_16 : 4;  // High bits of segment limit
0784   uint avl : 1;        // Unused (available for software use)
0785   uint rsv1 : 1;       // Reserved
0786   uint db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
0787   uint g : 1;          // Granularity: limit scaled by 4K when set
0788   uint base_31_24 : 8; // High bits of segment base address
0789 };
0790 
0791 
0792 
0793 
0794 
0795 
0796 
0797 
0798 
0799 
0800 // Normal segment
0801 #define SEG(type, base, lim, dpl) (struct segdesc)    \
0802 { ((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,      \
0803   ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
0804   (uint)(lim) >> 28, 0, 0, 1, 1, (uint)(base) >> 24 }
0805 #define SEG16(type, base, lim, dpl) (struct segdesc)  \
0806 { (lim) & 0xffff, (uint)(base) & 0xffff,              \
0807   ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
0808   (uint)(lim) >> 16, 0, 0, 1, 0, (uint)(base) >> 24 }
0809 #endif
0810 
0811 #define DPL_USER    0x3     // User DPL
0812 
0813 // Application segment type bits
0814 #define STA_X       0x8     // Executable segment
0815 #define STA_W       0x2     // Writeable (non-executable segments)
0816 #define STA_R       0x2     // Readable (executable segments)
0817 
0818 // System segment type bits
0819 #define STS_T32A    0x9     // Available 32-bit TSS
0820 #define STS_IG32    0xE     // 32-bit Interrupt Gate
0821 #define STS_TG32    0xF     // 32-bit Trap Gate
0822 
0823 // A virtual address 'la' has a three-part structure as follows:
0824 //
0825 // +--------10------+-------10-------+---------12----------+
0826 // | Page Directory |   Page Table   | Offset within Page  |
0827 // |      Index     |      Index     |                     |
0828 // +----------------+----------------+---------------------+
0829 //  \--- PDX(va) --/ \--- PTX(va) --/
0830 
0831 // page directory index
0832 #define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)
0833 
0834 // page table index
0835 #define PTX(va)         (((uint)(va) >> PTXSHIFT) & 0x3FF)
0836 
0837 // construct virtual address from indexes and offset
0838 #define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))
0839 
0840 // Page directory and page table constants.
0841 #define NPDENTRIES      1024    // # directory entries per page directory
0842 #define NPTENTRIES      1024    // # PTEs per page table
0843 #define PGSIZE          4096    // bytes mapped by a page
0844 
0845 #define PTXSHIFT        12      // offset of PTX in a linear address
0846 #define PDXSHIFT        22      // offset of PDX in a linear address
0847 
0848 #define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
0849 #define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
0850 // Page table/directory entry flags.
0851 #define PTE_P           0x001   // Present
0852 #define PTE_W           0x002   // Writeable
0853 #define PTE_U           0x004   // User
0854 #define PTE_PS          0x080   // Page Size
0855 #define PTE_LAZY        0x200   // Lazy allocation bit
0856 
0857 // Address in page table or page directory entry
0858 #define PTE_ADDR(pte)   ((uint)(pte) & ~0xFFF)
0859 #define PTE_FLAGS(pte)  ((uint)(pte) &  0xFFF)
0860 
0861 #ifndef __ASSEMBLER__
0862 typedef uint pte_t;
0863 
0864 // Task state segment format
0865 struct taskstate {
0866   uint link;         // Old ts selector
0867   uint esp0;         // Stack pointers and segment selectors
0868   ushort ss0;        //   after an increase in privilege level
0869   ushort padding1;
0870   uint *esp1;
0871   ushort ss1;
0872   ushort padding2;
0873   uint *esp2;
0874   ushort ss2;
0875   ushort padding3;
0876   void *cr3;         // Page directory base
0877   uint *eip;         // Saved state from last task switch
0878   uint eflags;
0879   uint eax;          // More saved state (registers)
0880   uint ecx;
0881   uint edx;
0882   uint ebx;
0883   uint *esp;
0884   uint *ebp;
0885   uint esi;
0886   uint edi;
0887   ushort es;         // Even more saved state (segment selectors)
0888   ushort padding4;
0889   ushort cs;
0890   ushort padding5;
0891   ushort ss;
0892   ushort padding6;
0893   ushort ds;
0894   ushort padding7;
0895   ushort fs;
0896   ushort padding8;
0897   ushort gs;
0898   ushort padding9;
0899   ushort ldt;
0900   ushort padding10;
0901   ushort t;          // Trap on task switch
0902   ushort iomb;       // I/O map base address
0903 };
0904 
0905 // Gate descriptors for interrupts and traps
0906 struct gatedesc {
0907   uint off_15_0 : 16;   // low 16 bits of offset in segment
0908   uint cs : 16;         // code segment selector
0909   uint args : 5;        // # args, 0 for interrupt/trap gates
0910   uint rsv1 : 3;        // reserved(should be zero I guess)
0911   uint type : 4;        // type(STS_{IG32,TG32})
0912   uint s : 1;           // must be 0 (system)
0913   uint dpl : 2;         // descriptor(meaning new) privilege level
0914   uint p : 1;           // Present
0915   uint off_31_16 : 16;  // high bits of offset in segment
0916 };
0917 
0918 // Set up a normal interrupt/trap gate descriptor.
0919 // - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
0920 //   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
0921 // - sel: Code segment selector for interrupt/trap handler
0922 // - off: Offset in code segment for interrupt/trap handler
0923 // - dpl: Descriptor Privilege Level -
0924 //        the privilege level required for software to invoke
0925 //        this interrupt/trap gate explicitly using an int instruction.
0926 #define SETGATE(gate, istrap, sel, off, d)                \
0927 {                                                         \
0928   (gate).off_15_0 = (uint)(off) & 0xffff;                \
0929   (gate).cs = (sel);                                      \
0930   (gate).args = 0;                                        \
0931   (gate).rsv1 = 0;                                        \
0932   (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
0933   (gate).s = 0;                                           \
0934   (gate).dpl = (d);                                       \
0935   (gate).p = 1;                                           \
0936   (gate).off_31_16 = (uint)(off) >> 16;                  \
0937 }
0938 
0939 #endif
0940 
0941 
0942 
0943 
0944 
0945 
0946 
0947 
0948 
0949 
