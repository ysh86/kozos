#ifndef _KOZOS_SYSCALL_H_INCLUDED_
#define _KOZOS_SYSCALL_H_INCLUDED_

#include "defines.h"

typedef enum {
    KZ_SYSCALL_TYPE_RUN = 0,
    KZ_SYSCALL_TYPE_EXIT,
    KZ_SYSCALL_TYPE_WAIT,
    KZ_SYSCALL_TYPE_SLEEP,
    KZ_SYSCALL_TYPE_WAKEUP,
    KZ_SYSCALL_TYPE_GETID,
    KZ_SYSCALL_TYPE_CHPRI,
    KZ_SYSCALL_TYPE_KMALLOC,
    KZ_SYSCALL_TYPE_KMFREE,
    KZ_SYSCALL_TYPE_SEND,
    KZ_SYSCALL_TYPE_RECV,
} kz_syscall_type_t;

typedef struct {
    union {
        struct {
            kz_func_t func;
            char *name;
            int priority;
            size_t stacksize;
            int argc;
            char **argv;
            kz_thread_id_t ret;
        } run;
        struct {
            int dummy;
        } exit;
        struct {
            int ret;
        } wait;
        struct {
            int ret;
        } sleep;
        struct {
            kz_thread_id_t id;
            int ret;
        } wakeup;
        struct {
            kz_thread_id_t ret;
        } getid;
        struct {
            int priority;
            int ret;
        } chpri;
        struct {
            size_t size;
            void *ret;
        } kmalloc;
        struct {
            char *p;
            int ret;
        } kmfree;
        struct {
            kz_msgbox_id_t id;
            size_t size;
            char *p;
            size_t ret;
        } send;
        struct {
            kz_msgbox_id_t id;
            size_t *sizep;
            char **pp;
            kz_thread_id_t ret;
        } recv;
    } un;
} kz_syscall_param_t;

#endif
