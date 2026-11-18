/* SPDX-License-Identifier: GPL-3.0-or-later */
.text
.macro ENTRY id
.p2align 2
.hidden trames_bionic_call_\id
.global trames_bionic_call_\id
.type trames_bionic_call_\id,%function
trames_bionic_call_\id:
    movz x16, #9
    movk x16, #1, lsl #16
    movz x17, #\id
    hlt #0x5452
    ret
.size trames_bionic_call_\id,.-trames_bionic_call_\id
.endm
ENTRY 1
ENTRY 2
ENTRY 3
ENTRY 4
ENTRY 5
ENTRY 6
ENTRY 7
ENTRY 8
ENTRY 9
ENTRY 10
ENTRY 11
ENTRY 12
ENTRY 13
.section .note.GNU-stack,"",%progbits
