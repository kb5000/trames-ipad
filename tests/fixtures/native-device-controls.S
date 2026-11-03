// SPDX-License-Identifier: GPL-3.0-or-later
.text
.global video_call
.type video_call,%function
video_call:
    movz x16, #7
    movk x16, #1, lsl #16
    mov x17, #1
    hlt #0x5452
    ret
.size video_call,.-video_call
.section .note.GNU-stack,"",@progbits
