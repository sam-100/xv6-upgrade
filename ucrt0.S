# User-space runtime entry point.
# exec() sets %esp to: [fake return pc][argc][argv]
# We forward argc/argv to main, and force process exit if main returns.

.globl start
start:
  movl 4(%esp), %eax    # argc
  movl 8(%esp), %edx    # argv
  pushl %edx
  pushl %eax
  call main
  addl $8, %esp
  call exit

hang:
  jmp hang
