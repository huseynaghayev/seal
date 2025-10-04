#ifndef ARENA_H
#define ARENA_H


#include "sealconf.h"


struct arena {
    struct arena *next;
    size_t used;
    seal_byte mem[];
};


struct arena *arena_new(struct arena *next);
void *arena_alloc(struct arena **pa, size_t size);
void arena_free(struct arena *a);


#endif /* ARENA_H */
