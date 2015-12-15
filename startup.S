@ startup
//    .cpu cortex-m4
//    .fpu softvfp
//    .thumb
    .section .text
    .global _start
//    .type _start,@function
    .align
_start:
    bl main

    // loop
    b .
