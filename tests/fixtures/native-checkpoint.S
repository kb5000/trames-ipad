// SPDX-License-Identifier: GPL-3.0-or-later
.text
.global native_copy
.type native_copy,%function
native_copy:
    movz x16, #1
    movk x16, #1, lsl #16
    movz x17, #1
    hlt #0x5452
    ret
.size native_copy,.-native_copy
.section .note.GNU-stack,"",@progbits
