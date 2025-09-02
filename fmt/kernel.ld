9650 /* Simple linker script for the JOS kernel.
9651    See the GNU ld 'info' manual ("info ld") to learn the syntax. */
9652 
9653 OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
9654 OUTPUT_ARCH(i386)
9655 ENTRY(_start)
9656 
9657 SECTIONS
9658 {
9659 	/* Link the kernel at this address: "." means the current address */
9660         /* Must be equal to KERNLINK */
9661 	. = 0x80100000;
9662 
9663 	.text : AT(0x100000) {
9664 		*(.text .stub .text.* .gnu.linkonce.t.*)
9665 	}
9666 
9667 	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
9668 
9669 	.rodata : {
9670 		*(.rodata .rodata.* .gnu.linkonce.r.*)
9671 	}
9672 
9673 	/* Include debugging information in kernel memory */
9674 	.stab : {
9675 		PROVIDE(__STAB_BEGIN__ = .);
9676 		*(.stab);
9677 		PROVIDE(__STAB_END__ = .);
9678 	}
9679 
9680 	.stabstr : {
9681 		PROVIDE(__STABSTR_BEGIN__ = .);
9682 		*(.stabstr);
9683 		PROVIDE(__STABSTR_END__ = .);
9684 	}
9685 
9686 	/* Adjust the address for the data segment to the next page */
9687 	. = ALIGN(0x1000);
9688 
9689 	/* Conventionally, Unix linkers provide pseudo-symbols
9690 	 * etext, edata, and end, at the end of the text, data, and bss.
9691 	 * For the kernel mapping, we need the address at the beginning
9692 	 * of the data section, but that's not one of the conventional
9693 	 * symbols, because the convention started before there was a
9694 	 * read-only rodata section between text and data. */
9695 	PROVIDE(data = .);
9696 
9697 
9698 
9699 
9700 	/* The data segment */
9701 	.data : {
9702 		*(.data)
9703 	}
9704 
9705 	PROVIDE(edata = .);
9706 
9707 	.bss : {
9708 		*(.bss)
9709 	}
9710 
9711 	PROVIDE(end = .);
9712 
9713 	/DISCARD/ : {
9714 		*(.eh_frame .note.GNU-stack)
9715 	}
9716 }
9717 
9718 
9719 
9720 
9721 
9722 
9723 
9724 
9725 
9726 
9727 
9728 
9729 
9730 
9731 
9732 
9733 
9734 
9735 
9736 
9737 
9738 
9739 
9740 
9741 
9742 
9743 
9744 
9745 
9746 
9747 
9748 
9749 
