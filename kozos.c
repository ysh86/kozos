#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

#define THREAD_NUM 6
#define THREAD_NAME_SIZE 15

/* thread context */
typedef struct _kz_context {
    uintptr_t sp;
} kz_context;

/* Task Controll Block(TCB) */
typedef struct _kz_thread {
    struct _kz_thread *next;
    char name[THREAD_NAME_SIZE + 1];
    char *stack;

    struct {
        kz_func_t func;
        int argc;
        char **argv;
    } init;

    struct {
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;
} kz_thread;

/* ready queue */
static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque;

static kz_thread *current;
static kz_thread threads[THREAD_NUM];
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];

void dispatch(kz_context *context);

static int getcurrent(void)
{
    if (current == NULL) {
        return -1;
    }

    readyque.head = current->next;
    if (readyque.head == NULL) {
        readyque.tail = NULL;
    }
    current->next = NULL;

    return 0;
}

static int putcurrent(void)
{
    if (current == NULL) {
        return -1;
    }

    if (readyque.tail) {
        readyque.tail->next = current;
    } else {
        readyque.head = current;
    }
    readyque.tail = current;

    return 0;
}

static void thread_end(void)
{
    kz_exit();
}

static void thread_init(kz_thread *thp)
{
    thp->init.func(thp->init.argc, thp->init.argv);
    thread_end();
}

static kz_thread_id_t thread_run(kz_func_t func, char *name, size_t stacksize,
    int argc, char *argv[])
{
    int i;
    kz_thread *thp;
    uint32_t *sp;
    extern char userstack;
    static char *thread_stack = &userstack;

    for (i = 0; i < THREAD_NUM; i++) {
        thp = &threads[i];
        if (!thp->init.func) {
            break;
        }
    }
    if (i == THREAD_NUM) {
        return -1;
    }

    memset(thp, 0, sizeof(*thp));

    strcpy(thp->name, name);
    thp->next = NULL;

    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;

    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize;

    thp->stack = thread_stack;

    sp = (uint32_t *)thp->stack;
    if (current) {
        *(--sp) = 0x01000000; // xPSR
        *(--sp) = (uintptr_t)thread_init; // pc
        *(--sp) = (uintptr_t)thread_end; // lr: return from thread_init()
        *(--sp) = 0; // r12
        *(--sp) = 0; // r3
        *(--sp) = 0; // r2
        *(--sp) = 0; // r1
        *(--sp) = (uintptr_t)thp; // r0: arg for thread_init()

        *(--sp) = (uintptr_t)0xfffffff9; // lr: exception return
    } else {
        *(--sp) = (uintptr_t)thread_init; // lr: normal return
    }
    *(--sp) = (uintptr_t)thp; // r12: arg for thread_init()
    *(--sp) = 0; // r11
    *(--sp) = 0; // r10
    *(--sp) = 0; // r9
    *(--sp) = 0; // r8
    *(--sp) = 0; // r7
    *(--sp) = 0; // r6
    *(--sp) = 0; // r5
    *(--sp) = 0; // r4

    thp->context.sp = (uintptr_t)sp;

    putcurrent(); // put the caller thread into queue
    current = thp;
    putcurrent(); // put the new thread into queue

    return (kz_thread_id_t)current;
}

static int thread_exit(void)
{
    // TODO free stack for reuse

    puts(current->name);
    puts(" EXIT.\n");
    memset(current, 0, sizeof(*current));
    return 0;
}

static void thread_intr(softvec_type_t type, uintptr_t sp);
static int setintr(softvec_type_t type, kz_handler_t handler)
{
    softvec_setintr(type, thread_intr);

    handlers[type] = handler;

    return 0;
}

static void call_function(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    switch (type) {
    case KZ_SYSCALL_TYPE_RUN: // kz_run()
        p->un.run.ret = thread_run(
            p->un.run.func,
            p->un.run.name,
            p->un.run.stacksize,
            p->un.run.argc,
            p->un.run.argv
            );
        break;
    case KZ_SYSCALL_TYPE_EXIT: // kz_exit()
        thread_exit();
        break;
    default:
        break;
    }
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    getcurrent();
    call_function(type, p);
}

static void schedule(void)
{
    if (!readyque.head) {
        kz_sysdown();
    }

    current = readyque.head;
}

static void syscall_intr(void)
{
    syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void)
{
    puts(current->name);
    puts(" DOWN.\n");
    getcurrent();
    thread_exit();
}

static void thread_intr(softvec_type_t type, uintptr_t sp)
{
    current->context.sp = sp;

    if (handlers[type]) {
        handlers[type]();
    }

    schedule();

    dispatch(&current->context);
}

/* lib functions */
void kz_start(kz_func_t func, char *name, size_t stacksize,
    int argc, char *argv[])
{
    current = NULL;

    readyque.head = readyque.tail = NULL;
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));

    setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
    setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

    current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);

    dispatch(&current->context);
}

void kz_sysdown(void)
{
    puts("system error!\n");
    while (1)
        ;
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    current->syscall.type = type;
    current->syscall.param = param;
    __asm__ __volatile__ ("svc #0");
}
