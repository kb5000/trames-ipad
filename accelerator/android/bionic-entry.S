/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Static data-size dispatch, before any C frame/register saves. No host/CPU
 * feature query, mutable flag, TLS access or architecture test. */
.text
.macro SIZED name, length=x2
.p2align 2
.hidden trames_bionic_\name
.global trames_bionic_\name
.type trames_bionic_\name,%function
trames_bionic_\name:
    cmp \length, #128
    b.lo trames_orig_\name
    b trames_bionic_large_\name
.size trames_bionic_\name,.-trames_bionic_\name
.endm
/* memcpy/memmove keep the original short/medium entry entirely untouched.
 * Their existing >96-byte forward/backward branches enter the large wrappers;
 * original fallback continuations restore the original scratch registers. */
SIZED memset
SIZED memcmp
SIZED memchr
SIZED memrchr
SIZED strnlen, x1
SIZED strncmp
SIZED strncpy
SIZED stpncpy
SIZED mempcpy
SIZED strlcat

.p2align 2
.hidden trames_bionic_strlen
.global trames_bionic_strlen
.type trames_bionic_strlen,%function
trames_bionic_strlen:
    /* Aligned words stay inside a guest page. Stop at the first NUL before
     * considering the next word, including strings beside a guard page. */
    tst x0, #7
    b.ne trames_bionic_unaligned_strlen
    ldr x8, [x0]
    mov x9, #0x0101010101010101
    sub x10, x8, x9
    bic x10, x10, x8
    and x10, x10, #0x8080808080808080
    cbnz x10, 1f
    ldr x8, [x0, #8]
    sub x10, x8, x9
    bic x10, x10, x8
    and x10, x10, #0x8080808080808080
    cbz x10, trames_bionic_long_strlen
    rbit x10, x10
    clz x0, x10
    lsr x0, x0, #3
    add x0, x0, #8
    ret
1:
    rbit x10, x10
    clz x0, x10
    lsr x0, x0, #3
    ret
.size trames_bionic_strlen,.-trames_bionic_strlen
.section .note.GNU-stack,"",%progbits
