#include "intr.h"

extern void interrupt(int type);

void intr_softerr(void)
{
    interrupt(SOFTVEC_TYPE_SOFTERR);
}

void intr_syscall(void)
{
    interrupt(SOFTVEC_TYPE_SYSCALL);
}

void intr_serintr(void)
{
    interrupt(SOFTVEC_TYPE_SERINTR);
}
