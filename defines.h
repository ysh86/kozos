#ifndef _DEFINES_H_INCLUDED_
#define _DEFINES_H_INCLUDED_

#define NULL ((void *)0)
#define SERIAL_DEFAULT_DEVICE 1

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef unsigned int size_t;
typedef unsigned int uintptr_t;

typedef uint32_t kz_thread_id_t;
typedef int (*kz_func_t)(int argc, char *argv[]);
typedef void (*kz_handler_t)(void);

typedef enum {
    MSGBOX_ID_CONSINPUT = 0,
    MSGBOX_ID_CONSOUTPUT,
    MSGBOX_ID_NUM,
} kz_msgbox_id_t;

#endif
