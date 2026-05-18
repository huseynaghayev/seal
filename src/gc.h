#ifndef GC_H
#define GC_H

typedef struct {
    int type;
    void *p;
} gc_obj;

typedef struct seal_state seal_state;

typedef struct {
    seal_state *S;
    int len;
    int cap;
    gc_obj *objs;
} gc;

void gc_register(gc *g, void *p, int type);
void gc_sweep(gc *g);

#endif /* GC_H */
