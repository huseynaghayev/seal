#ifndef VALUE_H
#define VALUE_H


#include <string.h>

#include "sealconf.h"
#include "gc.h"


#define SEAL_TNULL    0
#define SEAL_TBOOL    1
#define SEAL_TINT     2
#define SEAL_TFLOAT     3
#define SEAL_TSTRING    4
#define SEAL_TLIST   5
#define SEAL_TMAP    6
#define SEAL_TFUNCTION  7
#define SEAL_TUSERDATA  8


#define SEAL_IS_NULL(v) ((v).type == SEAL_TNULL)
#define SEAL_IS_BOOL(v) ((v).type == SEAL_TBOOL)
#define SEAL_IS_INT(v)  ((v).type == SEAL_TINT)
#define SEAL_IS_FLOAT(v)  ((v).type == SEAL_TFLOAT)
#define SEAL_IS_NUM(v)  (SEAL_IS_INT(v) || SEAL_IS_FLOAT(v))
#define SEAL_IS_STRING(v) ((v).type == SEAL_TSTRING)
#define SEAL_IS_LIST(v) ((v).type == SEAL_TLIST)
#define SEAL_IS_MAP(v)  ((v).type == SEAL_TMAP)
#define SEAL_IS_FUNC(v) ((v).type == SEAL_TFUNCTION)
#define SEAL_IS_USERDATA(v) ((v).type == SEAL_TUSERDATA)

#define SEAL_AS_BOOL(v) ((v).as.boolean)
#define SEAL_AS_INT(v)  ((v).as.integer)
#define SEAL_AS_FLOAT(v)  ((v).as.floating)
#define SEAL_AS_NUM(v)  (SEAL_IS_INT(v) ? SEAL_AS_INT(v) : SEAL_AS_FLOAT(v))
#define SEAL_AS_STRING(v) ((v).as.string)
#define SEAL_AS_STRINGVAL(v) (SEAL_AS_STRING(v)->val)
#define SEAL_AS_LIST(v) ((v).as.list)
#define SEAL_AS_MAP(v)  ((v).as.map)
#define SEAL_AS_FUNC(v)  ((v).as.func)
#define SEAL_AS_SFUNC(v) (SEAL_AS_FUNC(v)->as.s)
#define SEAL_AS_CFUNC(v) (SEAL_AS_FUNC(v)->as.c)
#define SEAL_AS_USERDATA(v) ((v).as.udata)

#define SEAL_VAL(t, f, v) ((struct seal_value) { .type = t, .as.f = v })
#define SEAL_VNULL ((struct seal_value) { SEAL_TNULL })
#define SEAL_VBOOL(v)  SEAL_VAL(SEAL_TBOOL, boolean, v)
#define SEAL_VINT(v)   SEAL_VAL(SEAL_TINT, integer, v)
#define SEAL_VFLOAT(v) SEAL_VAL(SEAL_TFLOAT, floating, v)
#define SEAL_VSTRING(v) SEAL_VAL(SEAL_TSTRING, string, v)
#define SEAL_VLIST(v) SEAL_VAL(SEAL_TLIST, list, v)
#define SEAL_VMAP(v)  SEAL_VAL(SEAL_TMAP, map, v)
#define SEAL_VFUNC(v) SEAL_VAL(SEAL_TFUNCTION, func, v)
#define SEAL_VUSERDATA(v) SEAL_VAL(SEAL_TUSERDATA, udata, v)


/* forward declarations */
struct seal_string;
struct seal_list;
struct seal_hashmap;
struct seal_func;

int seal_format_float(seal_float f, char *buf, int bufsiz);

struct seal_value {
    /* ... */
    int type;
    union {
        seal_int   integer;
        seal_float floating;
        seal_bool  boolean;
        struct seal_string  *string;
        struct seal_list    *list;
        struct seal_hashmap *map;
        struct seal_func *func; /* prototype */
        void *udata;
    } as;
};

void seal_print_val(struct seal_value *v, bool inside_obj);

#define GC_Header \
    bool marked

#define Array_Specs \
    int cap; \
    int len

/* string */
struct seal_string {
    GC_Header;
    bool is_const;
    int len;
    const char *val;
};

const char *string_duplen(const char *s, int len);
const char *string_concat(const char *a, const char *b);
struct seal_string *string_new(const char *s, bool dup, bool is_const, gc *g);

#define string_dup(s) string_duplen(s, -1)

/* list */
struct seal_list {
    GC_Header;
    Array_Specs;
    struct seal_value *vals;
};

struct seal_list *list_new(int cap, gc *g);
void list_pushval(struct seal_list *l, struct seal_value v);

/* hashmap */
#define nullhentry(e) ((e) == NULL || (e)->key == NULL)

struct h_entry {
    unsigned int hash;
    const char *key;
    int keysize;
    struct seal_value val;
    bool is_tomb;
};

struct seal_hashmap {
    GC_Header;
    Array_Specs; /* specs for entries */
    struct h_entry *entries;
};

struct seal_hashmap *hashmap_new(int cap, gc *g);

struct h_entry *hashmap_searchlen(struct seal_hashmap *map,
                                  const char *key,
                                  int len);

/* key must be owned carefully outside */
int hashmap_insert(struct seal_hashmap *map,
                   const char *key,
                   struct seal_value val);

int hashmap_insert_e(struct seal_hashmap *map,
                     struct h_entry *entry,
                     const char *key,
                     struct seal_value val);

int hashmap_remove(struct seal_hashmap *map, const char *key);
int hashmap_free(struct seal_hashmap *map, bool free_key);


#define hashmap_Nnew(cap) hashmap_new(cap, NULL)

#define hashmap_search(map, key) hashmap_searchlen(map, key, -1)

struct seal_state;
typedef void (*seal_Cfunction) (struct seal_state *S);

/* function */

#define FUNCTION_TYPE_SEAL 0
#define FUNCTION_TYPE_C  1

struct seal_func {
    int type;
    union {
        /* Seal function */
        struct {
            const char *name;  /* NULL ptr if anonymous */
            const char *file_name; /* the file name it is defined in */
            int line; /* the line which it is defined at */
            seal_byte psize;   /* number of parameters */
            struct chunk *c; /* chunk */
        } s;
        /* C function */
        struct {
            const char *name;  /* NULL ptr if anonymous */
            seal_Cfunction f; /* C function pointer */
        } c;
    } as;
};

/*
struct seal_func *func_new(const char *name,
                           seal_byte *body,
                           seal_byte psize,
                           seal_byte locsize);
*/

#endif /* VALUE_H */
