#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

/* thread context */
typedef struct _kz_context {
    uintptr_t sp;
} kz_context;

/* Task Controll Block(TCB) */
typedef struct _kz_thread {
    struct _kz_thread *next;
    char name[THREAD_NAME_SIZE + 1];
    int priority;
    char *stack;
    uint32_t flags;
#define KZ_THREAD_FLAG_READY (1<<0)

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

/* message buffer */
typedef struct _kz_msgbuf {
    struct _kz_msgbuf *next;
    kz_thread *sender;
    struct {
        size_t size;
        char *p;
    } param;
} kz_msgbuf;

/* message box */
typedef struct _kz_msgbox {
    kz_thread *receiver;
    kz_msgbuf *head;
    kz_msgbuf *tail;

    uint32_t dummy_for_alignment[1];
} kz_msgbox;

/* ready queue */
static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque[PRIORITY_NUM];

static kz_thread *current;
static kz_thread threads[THREAD_NUM];
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];
static kz_msgbox msgboxes[MSGBOX_ID_NUM];

void dispatch(kz_context *context);

static int getcurrent(void)
{
    if (current == NULL) {
        return -1;
    }
    if (!(current->flags & KZ_THREAD_FLAG_READY)) {
        return 1;
    }

    readyque[current->priority].head = current->next;
    if (readyque[current->priority].head == NULL) {
        readyque[current->priority].tail = NULL;
    }
    current->flags &= ~KZ_THREAD_FLAG_READY;
    current->next = NULL;

    return 0;
}

static int putcurrent(void)
{
    if (current == NULL) {
        return -1;
    }
    if (current->flags & KZ_THREAD_FLAG_READY) {
        return 1;
    }

    if (readyque[current->priority].tail) {
        readyque[current->priority].tail->next = current;
    } else {
        readyque[current->priority].head = current;
    }
    readyque[current->priority].tail = current;
    current->flags |= KZ_THREAD_FLAG_READY;

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

static kz_thread_id_t thread_run(kz_func_t func, char *name, int priority, size_t stacksize,
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
    thp->priority = priority;
    thp->flags = 0;

    // TODO
    // if (priority == 0) then interrupts should be prohibited

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

static int thread_wait(void)
{
    putcurrent();
    return 0;
}

static int thread_sleep(void)
{
    return 0;
}

static int thread_wakeup(kz_thread_id_t id)
{
    putcurrent();

    current = (kz_thread *)id;
    putcurrent();

    return 0;
}

static kz_thread_id_t thread_getid(void)
{
    putcurrent();
    return (kz_thread_id_t)current;
}

static int thread_chpri(int priority)
{
    int old = current->priority;
    if (priority >= 0) {
        current->priority = priority;
    }
    putcurrent();
    return old;
}

static void *thread_kmalloc(size_t size)
{
    putcurrent();
    return kzmem_alloc(size);
}

static int thread_kmfree(char *p)
{
    kzmem_free(p);
    putcurrent();
    return 0;
}

static void sendmsg(kz_msgbox *mboxp, kz_thread *thp, size_t size, char *p)
{
    kz_msgbuf *mp;

    mp = (kz_msgbuf *)kzmem_alloc(sizeof(*mp));
    if (mp == NULL) {
        kz_sysdown();
    }
    mp->next       = NULL;
    mp->sender     = thp;
    mp->param.size = size;
    mp->param.p    = p;

    if (mboxp->tail) {
        mboxp->tail->next = mp;
    } else {
        mboxp->head = mp;
    }
    mboxp->tail = mp;
}

static void recvmsg(kz_msgbox *mboxp)
{
    kz_msgbuf *mp;
    kz_syscall_param_t *p;

    mp = mboxp->head;
    mboxp->head = mp->next;
    if (mboxp->head == NULL) {
        mboxp->tail = NULL;
    }
    mp->next = NULL;

    p = mboxp->receiver->syscall.param;
    p->un.recv.ret = (kz_thread_id_t)mp->sender;
    if (p->un.recv.sizep) {
        *(p->un.recv.sizep) = mp->param.size;
    }
    if (p->un.recv.pp) {
        *(p->un.recv.pp) = mp->param.p;
    }

    mboxp->receiver = NULL;

    kzmem_free(mp);
}

static size_t thread_send(kz_msgbox_id_t id, size_t size, char *p)
{
    kz_msgbox *mboxp = &msgboxes[id];

    putcurrent();
    sendmsg(mboxp, current, size, p);

    if (mboxp->receiver) {
        current = mboxp->receiver;
        recvmsg(mboxp);
        putcurrent();
    }

    return size;
}

static kz_thread_id_t thread_recv(kz_msgbox_id_t id, size_t *sizep, char **pp)
{
    kz_msgbox *mboxp = &msgboxes[id];

    if (mboxp->receiver) {
        kz_sysdown();
    }

    mboxp->receiver = current;

    if (mboxp->head == NULL) {
        return -1; // msgbox is empty so receiver goes to sleep.
    }

    recvmsg(mboxp);
    putcurrent();

    return current->syscall.param->un.recv.ret;
}

static void thread_intr(softvec_type_t type, uintptr_t sp);
static int thread_setintr(softvec_type_t type, kz_handler_t handler)
{
    softvec_setintr(type, thread_intr);

    handlers[type] = handler;
    putcurrent();

    return 0;
}

static void call_function(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    switch (type) {
    case KZ_SYSCALL_TYPE_RUN: // kz_run()
        p->un.run.ret = thread_run(
            p->un.run.func,
            p->un.run.name,
            p->un.run.priority,
            p->un.run.stacksize,
            p->un.run.argc,
            p->un.run.argv
            );
        break;
    case KZ_SYSCALL_TYPE_EXIT: // kz_exit()
        thread_exit();
        break;
    case KZ_SYSCALL_TYPE_WAIT:
        p->un.wait.ret = thread_wait();
        break;
    case KZ_SYSCALL_TYPE_SLEEP:
        p->un.sleep.ret = thread_sleep();
        break;
    case KZ_SYSCALL_TYPE_WAKEUP:
        p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
        break;
    case KZ_SYSCALL_TYPE_GETID:
        p->un.getid.ret = thread_getid();
        break;
    case KZ_SYSCALL_TYPE_CHPRI:
        p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
        break;
    case KZ_SYSCALL_TYPE_KMALLOC:
        p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
        break;
    case KZ_SYSCALL_TYPE_KMFREE:
        p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
        break;
    case KZ_SYSCALL_TYPE_SEND:
        p->un.send.ret = thread_send(p->un.send.id, p->un.send.size, p->un.send.p);
        break;
    case KZ_SYSCALL_TYPE_RECV:
        p->un.recv.ret = thread_recv(p->un.recv.id, p->un.recv.sizep, p->un.recv.pp);
        break;
    case KZ_SYSCALL_TYPE_SETINTR:
        p->un.setintr.ret = thread_setintr(p->un.setintr.type, p->un.setintr.handler);
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

static void srvcall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    current = NULL;
    call_function(type, p);
}

static void schedule(void)
{
    int i;

    for (i = 0; i < PRIORITY_NUM; i++) {
        if (readyque[i].head) {
            break;
        }
    }
    if (i == PRIORITY_NUM) {
        kz_sysdown();
    }

    current = readyque[i].head;
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
void kz_start(kz_func_t func, char *name, int priority, size_t stacksize,
    int argc, char *argv[])
{
    kzmem_init();

    current = NULL;

    memset(readyque, 0, sizeof(readyque));
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));
    memset(msgboxes, 0, sizeof(msgboxes));

    thread_setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
    thread_setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

    current = (kz_thread *)thread_run(func, name, priority, stacksize, argc, argv);

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

void kz_srvcall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    srvcall_proc(type, param);
}
