#include "arena.h"


#define BLOCK_SIZE 4096 /* 4 KBs */


struct arena *arena_new(struct arena *next)
{
    struct arena *a = SEAL_MALLOC(sizeof(struct arena) + BLOCK_SIZE);

    a->next = next;
    a->used = 0;

    return a;
}

void *arena_alloc(struct arena **pa, size_t size)
{
    if (size > BLOCK_SIZE)
        return NULL;

    /* align with pointer size */
    size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

    struct arena *a = *pa;
    if (!a || a->used + size > BLOCK_SIZE) {
        a = arena_new(a);
        *pa = a;
    }

    void *ptr = a->mem + a->used;
    a->used += size;
    return ptr;
}

void arena_free(struct arena *a)
{
    while (a) {
        struct arena *next = a->next;
        SEAL_FREE(a);
        a = next;
    }
}
