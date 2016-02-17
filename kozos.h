#ifndef _KOZOS_H_INCLUDED_
#define _KOZOS_H_INCLUDED_

#include "defines.h"
#include "interrupt.h"
#include "syscall.h"

/* system call */
kz_thread_id_t kz_run(kz_func_t func, char *name, int priority, size_t stacksize,
    int argc, char *argv[]);
void kz_exit(void);
int kz_wait(void);
int kz_sleep(void);
int kz_wakeup(kz_thread_id_t id);
kz_thread_id_t kz_getid(void);
int kz_chpri(int priority);
void *kz_kmalloc(size_t size);
int kz_kmfree(void *p);
size_t kz_send(kz_msgbox_id_t id, size_t size, char *p);
kz_thread_id_t kz_recv(kz_msgbox_id_t id, size_t *sizep, char **pp);
int kz_setintr(softvec_type_t type, kz_handler_t handler);

/* service call */
int kx_wakeup(kz_thread_id_t id);
void *kx_kmalloc(size_t size);
int kx_kmfree(void *p);
size_t kx_send(kz_msgbox_id_t id, size_t size, char *p);

/* lib functions */
void kz_start(kz_func_t func, char *name, int priority, size_t stacksize,
    int argc, char *argv[]);
void kz_sysdown(void);
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param);
void kz_srvcall(kz_syscall_type_t type, kz_syscall_param_t *param);

/* system task */
int consdrv_main(int argc, char *argv[]);

/* user task */
int command_main(int argc, char *argv[]);

#endif
