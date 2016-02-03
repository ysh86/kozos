#include "intr.h"
    .syntax unified
    .section .text

    .global _intr_softerr
    .type _intr_softerr,%function
_intr_softerr:
    push {r4-r12,lr}
    mov r1,sp
    ldr sp,=_intrstack
    push {r1,r12}
    mov r0,#SOFTVEC_TYPE_SOFTERR
    bl _interrupt
    pop {r1,r12}
    mov sp,r1
    pop {r4-r12,lr}
    bx lr

    .global _intr_syscall
    .type _intr_syscall,%function
_intr_syscall:
    push {r4-r12,lr}
    mov r1,sp
    ldr sp,=_intrstack
    push {r1,r12}
    mov r0,#SOFTVEC_TYPE_SYSCALL
    bl _interrupt
    pop {r1,r12}
    mov sp,r1
    pop {r4-r12,lr}
    bx lr

    .global _intr_serintr
    .type _intr_serintr,%function
_intr_serintr:
    push {r4-r12,lr}
    mov r1,sp
    ldr sp,=_intrstack
    push {r1,r12}
    mov r0,#SOFTVEC_TYPE_SERINTR
    bl _interrupt
    pop {r1,r12}
    mov sp,r1
    pop {r4-r12,lr}
    bx lr
