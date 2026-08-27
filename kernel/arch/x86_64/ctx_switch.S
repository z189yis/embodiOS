/* x86_64 Task Context Switch
 *
 * void ctx_switch(uint64_t *save_slot, uint64_t new_sp)
 *
 * Saves the current execution context (callee-saved registers, RFLAGS and
 * the return address) onto the current stack and records its stack pointer
 * in *save_slot. Then loads new_sp and resumes whatever context it holds:
 * a suspended task's frame, or the initial frame built by task_create()
 * whose "return address" points at the task entry trampoline.
 *
 * Registers saved per System V AMD64 ABI callee-saved set:
 *   RBX, RBP, R12-R15 (+RSP), plus RFLAGS for interrupt state.
 * Caller-saved registers need no saving: the switch point is a C function
 * call boundary, so they are already dead.
 */
.section .text

.global ctx_switch

ctx_switch:
    pushfq                          /* flags restored on resume */
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq  %rsp, (%rdi)              /* remember where the old task stopped */

    movq  %rsi, %rsp                /* adopt the next task's stack */
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    popfq
    ret                             /* jump into the next task */
