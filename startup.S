@ startup
//    .cpu cortex-m4
//    .fpu softvfp
//    .thumb
    .syntax unified
    .section .text
    .global _start
    .type _start,%function
    .align
_start:
    bl _main

    // loop
    b .

    .global _dispatch
    .type _dispatch,%function
_dispatch:
    ldr sp,[r0]
    pop {r4-r12,lr}
    mov r0,r12
    bx lr
