#ifndef _INTERRUPT_H_INCLUDED_
#define _INTERRUPT_H_INCLUDED_

extern char softvec;
#define SOFTVEC_ADDR (&softvec)

typedef int softvec_type_t;

typedef void (*softvec_handler_t)(softvec_type_t type);

#define SOFTVECS ((softvec_handler_t *)SOFTVEC_ADDR)

#define INTR_ENABLE  __asm__ __volatile__ ("cpsie i")
#define INTR_DISABLE __asm__ __volatile__ ("cpsid i")

int softvec_init(void);

int softvec_setintr(softvec_type_t type, softvec_handler_t handler);

void interrupt(softvec_type_t type);

#endif
