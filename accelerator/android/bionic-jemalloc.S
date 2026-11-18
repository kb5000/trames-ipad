/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Internal block adapters, not replacement host allocators. Preserve every
 * live original caller-saved register and resume the original Bionic frame. */
.text
.macro SAVE
    sub sp, sp, #48
    stp x0, x1, [sp]
    stp x2, x30, [sp, #16]
    stp x16, x17, [sp, #32]
.endm
.macro RESTORE
    ldp x0, x1, [sp]
    ldp x2, x30, [sp, #16]
    ldp x16, x17, [sp, #32]
    add sp, sp, #48
.endm
.p2align 2
.hidden trames_jemalloc_pop
.global trames_jemalloc_pop
.type trames_jemalloc_pop,%function
trames_jemalloc_pop:
    SAVE
    mov x0, x19
    mov w1, w21
    adrp x2, trames_jemalloc_sizes
    add x2, x2, :lo12:trames_jemalloc_sizes
    bl trames_bionic_call_11
    cbnz x1, 1f
    mov x20, x0
    RESTORE
    b trames_jemalloc_pop_resume
1:
    RESTORE
    b trames_jemalloc_pop_original
.size trames_jemalloc_pop,.-trames_jemalloc_pop
.p2align 2
.hidden trames_jemalloc_push
.global trames_jemalloc_push
.type trames_jemalloc_push,%function
trames_jemalloc_push:
    SAVE
    mov x0, x19
    // Flush can clobber x3. The original block retains its bin in x23.
    add x1, x23, #0x1d0
    mov x2, x20
    bl trames_bionic_call_12
    cbnz x1, 1f
    RESTORE
    b trames_jemalloc_push_resume
1:
    RESTORE
    b trames_jemalloc_push_original
.size trames_jemalloc_push,.-trames_jemalloc_push
.section .note.GNU-stack,"",%progbits
