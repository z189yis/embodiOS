/* aarch64 Task Context Switch
 *
 * uint64_t ctx_switch(uint64_t *save_slot, uint64_t new_sp)
 *
 * x0 = save_slot, x1 = new_sp ( AAPCS64 calling convention )
 *
 * Saves callee-saved x19-x28, FP/LR (x29/x30) and DAIF flags onto the
 * current stack, records the stack pointer in *save_slot, then restores
 * the frame at new_sp and branches to its saved LR. task_create() builds
 * the initial frame with the entry trampoline in the x30 slot.
 *
 * NOTE: built for symmetry with the x86_64 path; this kernel's CI boots
 * x86_64 only, so this file has no boot-time coverage yet.
 */
.section .text

.global ctx_switch

ctx_switch:
    /* Save callee-saved pairs (sp stays 16-byte aligned throughout) */
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!

    /* Save interrupt mask state + padding */
    mrs x9, daif
    stp x9, xzr, [sp, #-16]!

    /* Save frame pointer + return address */
    stp x29, x30, [sp, #-16]!

    /* Record where the old task stopped */
    mov x9, sp
    str x9, [x0]

    /* Adopt the next task's stack */
    mov sp, x1

    ldp x29, x30, [sp], #16
    ldp x9, xzr, [sp], #16
    msr daif, x9

    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16

    ret
