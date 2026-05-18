#include "gc.h"
#include "sealconf.h"
#include "value.h"
#include "state.h"

#define GC_START_CAP 256

void gc_register(gc *g, void *p, int type)
{
    if (!g->objs || !g->cap) {
        g->objs = SEAL_MALLOC(sizeof(gc_obj) * GC_START_CAP);
        g->cap = GC_START_CAP;
        g->len = 0;
    } else if (g->len >= g->cap) {
        seal_gc(g->S);
        if (g->len >= g->cap) {
            g->cap *= 2;
            g->objs = SEAL_REALLOC(g->objs, sizeof(gc_obj) * g->cap);
        }
    }
    g->objs[g->len++] = (gc_obj) { type, p };
}

void gc_sweep(gc *g)
{
    int alive = 0;
    struct seal_string *s;
    struct seal_list *l;
    struct seal_hashmap *m;
    for (int i = 0; i < g->len; i++) {
        gc_obj *go = &g->objs[i];
        bool marked;
        switch (go->type) {
        case SEAL_TSTRING:
            s = go->p;
            marked = s->marked;
            if (marked) {
                s->marked = false;    
            } else {
                SEAL_FREE((void *)s->val);
                SEAL_FREE(s);
            }
            break;
        case SEAL_TLIST:
            l = go->p;
            marked = l->marked;
            if (marked) {
                l->marked = false;    
            } else {
                SEAL_FREE(l->vals);
                SEAL_FREE(l);
            }
            break;
        case SEAL_TMAP:
            m = go->p;
            marked = m->marked;
            if (marked) {
                m->marked = false;    
            } else {
                hashmap_free(m, false);
            }
            break;
        }
        if (marked)
            g->objs[alive++] = *go;
    }
    g->len = alive;
}
