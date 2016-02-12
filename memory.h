#ifndef _KOZOS_MEMORY_H_INCLUDED_
#define _KOZOS_MEMORY_H_INCLUDED_

int kzmem_init(void);
void *kzmem_alloc(size_t size);
void kzmem_free(void *mem);

#endif
