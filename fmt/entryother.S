1150 #include "asm.h"
1151 #include "memlayout.h"
1152 #include "mmu.h"
1153 
1154 # Each non-boot CPU ("AP") is started up in response to a STARTUP
1155 # IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
1156 # Specification says that the AP will start in real mode with CS:IP
1157 # set to XY00:0000, where XY is an 8-bit value sent with the
1158 # STARTUP. Thus this code must start at a 4096-byte boundary.
1159 #
1160 # Because this code sets DS to zero, it must sit
1161 # at an address in the low 2^16 bytes.
1162 #
1163 # Startothers (in main.c) sends the STARTUPs one at a time.
1164 # It copies this code (start) at 0x7000.  It puts the address of
1165 # a newly allocated per-core stack in start-4,the address of the
1166 # place to jump to (mpenter) in start-8, and the physical address
1167 # of entrypgdir in start-12.
1168 #
1169 # This code combines elements of bootasm.S and entry.S.
1170 
1171 .code16
1172 .globl start
1173 start:
1174   cli
1175 
1176   # Zero data segment registers DS, ES, and SS.
1177   xorw    %ax,%ax
1178   movw    %ax,%ds
1179   movw    %ax,%es
1180   movw    %ax,%ss
1181 
1182   # Switch from real to protected mode.  Use a bootstrap GDT that makes
1183   # virtual addresses map directly to physical addresses so that the
1184   # effective memory map doesn't change during the transition.
1185   lgdt    gdtdesc
1186   movl    %cr0, %eax
1187   orl     $CR0_PE, %eax
1188   movl    %eax, %cr0
1189 
1190   # Complete the transition to 32-bit protected mode by using a long jmp
1191   # to reload %cs and %eip.  The segment descriptors are set up with no
1192   # translation, so that the mapping is still the identity mapping.
1193   ljmpl    $(SEG_KCODE<<3), $(start32)
1194 
1195 
1196 
1197 
1198 
1199 
1200 .code32  # Tell assembler to generate 32-bit code now.
1201 start32:
1202   # Set up the protected-mode data segment registers
1203   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
1204   movw    %ax, %ds                # -> DS: Data Segment
1205   movw    %ax, %es                # -> ES: Extra Segment
1206   movw    %ax, %ss                # -> SS: Stack Segment
1207   movw    $0, %ax                 # Zero segments not ready for use
1208   movw    %ax, %fs                # -> FS
1209   movw    %ax, %gs                # -> GS
1210 
1211   # Turn on page size extension for 4Mbyte pages
1212   movl    %cr4, %eax
1213   orl     $(CR4_PSE), %eax
1214   movl    %eax, %cr4
1215   # Use entrypgdir as our initial page table
1216   movl    (start-12), %eax
1217   movl    %eax, %cr3
1218   # Turn on paging.
1219   movl    %cr0, %eax
1220   orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
1221   movl    %eax, %cr0
1222 
1223   # Switch to the stack allocated by startothers()
1224   movl    (start-4), %esp
1225   # Call mpenter()
1226   call	 *(start-8)
1227 
1228   movw    $0x8a00, %ax
1229   movw    %ax, %dx
1230   outw    %ax, %dx
1231   movw    $0x8ae0, %ax
1232   outw    %ax, %dx
1233 spin:
1234   jmp     spin
1235 
1236 .p2align 2
1237 gdt:
1238   SEG_NULLASM
1239   SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
1240   SEG_ASM(STA_W, 0, 0xffffffff)
1241 
1242 
1243 gdtdesc:
1244   .word   (gdtdesc - gdt - 1)
1245   .long   gdt
1246 
1247 
1248 
1249 
