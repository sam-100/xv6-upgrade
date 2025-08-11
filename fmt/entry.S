1050 # The xv6 kernel starts executing in this file. This file is linked with
1051 # the kernel C code, so it can refer to kernel symbols such as main().
1052 # The boot block (bootasm.S and bootmain.c) jumps to entry below.
1053 
1054 # Multiboot header, for multiboot boot loaders like GNU Grub.
1055 # http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
1056 #
1057 # Using GRUB 2, you can boot xv6 from a file stored in a
1058 # Linux file system by copying kernel or kernelmemfs to /boot
1059 # and then adding this menu entry:
1060 #
1061 # menuentry "xv6" {
1062 # 	insmod ext2
1063 # 	set root='(hd0,msdos1)'
1064 # 	set kernel='/boot/kernel'
1065 # 	echo "Loading ${kernel}..."
1066 # 	multiboot ${kernel} ${kernel}
1067 # 	boot
1068 # }
1069 
1070 #include "asm.h"
1071 #include "memlayout.h"
1072 #include "mmu.h"
1073 #include "param.h"
1074 
1075 # Multiboot header.  Data to direct multiboot loader.
1076 .p2align 2
1077 .text
1078 .globl multiboot_header
1079 multiboot_header:
1080   #define magic 0x1badb002
1081   #define flags 0
1082   .long magic
1083   .long flags
1084   .long (-magic-flags)
1085 
1086 # By convention, the _start symbol specifies the ELF entry point.
1087 # Since we haven't set up virtual memory yet, our entry point is
1088 # the physical address of 'entry'.
1089 .globl _start
1090 _start = V2P_WO(entry)
1091 
1092 # Entering xv6 on boot processor, with paging off.
1093 .globl entry
1094 entry:
1095   # Turn on page size extension for 4Mbyte pages
1096   movl    %cr4, %eax
1097   orl     $(CR4_PSE), %eax
1098   movl    %eax, %cr4
1099   # Set page directory
1100   movl    $(V2P_WO(entrypgdir)), %eax
1101   movl    %eax, %cr3
1102   # Turn on paging.
1103   movl    %cr0, %eax
1104   orl     $(CR0_PG|CR0_WP), %eax
1105   movl    %eax, %cr0
1106 
1107   # Set up the stack pointer.
1108   movl $(stack + KSTACKSIZE), %esp
1109 
1110   # Jump to main(), and switch to executing at
1111   # high addresses. The indirect call is needed because
1112   # the assembler produces a PC-relative instruction
1113   # for a direct jump.
1114   mov $main, %eax
1115   jmp *%eax
1116 
1117 .comm stack, KSTACKSIZE
1118 
1119 
1120 
1121 
1122 
1123 
1124 
1125 
1126 
1127 
1128 
1129 
1130 
1131 
1132 
1133 
1134 
1135 
1136 
1137 
1138 
1139 
1140 
1141 
1142 
1143 
1144 
1145 
1146 
1147 
1148 
1149 
