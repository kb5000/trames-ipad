/* SPDX-License-Identifier: GPL-3.0-or-later */
.text
.p2align 2
.macro entry id
.hidden trames_glibc_call_\id
.global trames_glibc_call_\id
.type trames_glibc_call_\id,%function
trames_glibc_call_\id:
 movz x16,#10
 movk x16,#1,lsl#16
 movz x17,#\id
 hlt #0x5452
 ret
.size trames_glibc_call_\id,.-trames_glibc_call_\id
.endm
entry 1
entry 2
.section .note.GNU-stack,"",%progbits
