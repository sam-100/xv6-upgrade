3350 # Context switch
3351 #
3352 #   void swtch(struct context **old, struct context *new);
3353 #
3354 # Save the current registers on the stack, creating
3355 # a struct context, and save its address in *old.
3356 # Switch stacks to new and pop previously-saved registers.
3357 
3358 .globl swtch
3359 swtch:
3360   movl 4(%esp), %eax
3361   movl 8(%esp), %edx
3362 
3363   # Save old callee-saved registers
3364   pushl %ebp
3365   pushl %ebx
3366   pushl %esi
3367   pushl %edi
3368 
3369   # Switch stacks
3370   movl %esp, (%eax)
3371   movl %edx, %esp
3372 
3373   # Load new callee-saved registers
3374   popl %edi
3375   popl %esi
3376   popl %ebx
3377   popl %ebp
3378   ret
3379 
3380 
3381 
3382 
3383 
3384 
3385 
3386 
3387 
3388 
3389 
3390 
3391 
3392 
3393 
3394 
3395 
3396 
3397 
3398 
3399 
