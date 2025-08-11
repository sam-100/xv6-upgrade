0700 //
0701 // assembler macros to create x86 segments
0702 //
0703 
0704 #define SEG_NULLASM                                             \
0705         .word 0, 0;                                             \
0706         .byte 0, 0, 0, 0
0707 
0708 // The 0xC0 means the limit is in 4096-byte units
0709 // and (for executable segments) 32-bit mode.
0710 #define SEG_ASM(type,base,lim)                                  \
0711         .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);      \
0712         .byte (((base) >> 16) & 0xff), (0x90 | (type)),         \
0713                 (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)
0714 
0715 #define STA_X     0x8       // Executable segment
0716 #define STA_W     0x2       // Writeable (non-executable segments)
0717 #define STA_R     0x2       // Readable (executable segments)
0718 
0719 
0720 
0721 
0722 
0723 
0724 
0725 
0726 
0727 
0728 
0729 
0730 
0731 
0732 
0733 
0734 
0735 
0736 
0737 
0738 
0739 
0740 
0741 
0742 
0743 
0744 
0745 
0746 
0747 
0748 
0749 
