#ifndef VALUE_H
#define VALUE_H


#include <string.h>

#include "sealconf.h"


/* forward declarations */
struct seal_string;
struct seal_list;
struct seal_hashmap;


struct seal_value {
    /* ... */
    union {
        seal_int       integer;
        seal_float     floating;
        seal_bool      boolean;
        struct seal_string  *string;
        struct seal_list    *list;
        struct seal_hashmap *map;
    } as;
};

#define GC_Header \
    int ref_count; \
    bool collect

#define Array_Specs \
    int cap; \
    int len

/* string */
struct seal_string {
    GC_Header;
    int len;
    const char *val;
};

const char *string_duplen(const char *s, int len);
const char *string_concat(const char *a, const char *b);
struct seal_string *string_new(const char *s, bool collect, bool dup);

#define string_dup(s) string_duplen(s, -1)

#define string_CDnew(s) string_new(s, true, true)
#define string_Cnew(s)  string_new(s, true, false)
#define string_Nnew(s)  string_new(s, false, false)

/* list */
struct seal_list {
    GC_Header;
    Array_Specs;
    struct seal_value vals[];
};

struct seal_list *list_new(int cap);

/* hashmap */
#define nullhentry(e) ((e) == NULL || (e)->key == NULL)

struct h_entry {
    int hash;
    const char *key;
    struct seal_value val;
    bool is_tomb;
};

struct seal_hashmap {
    GC_Header;
    Array_Specs; /* specs for entries */
    struct h_entry *entries;
};

struct seal_hashmap *hashmap_new(int cap, bool collect);
struct h_entry *hashmap_searchlen(struct seal_hashmap *map, const char *key, int len);
int hashmap_insert(struct seal_hashmap *map, const char *key, struct seal_value val);
int hashmap_insert_e(struct seal_hashmap *map, struct h_entry *entry, const char *key, struct seal_value val);


#define hashmap_Cnew(cap) hashmap_new(cap, true)
#define hashmap_Nnew(cap) hashmap_new(cap, false)

#define hashmap_search(map, key) hashmap_searchlen(map, key, -1)

#endif /* VALUE_H */
