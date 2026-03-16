#include "value.h"
#include <stdio.h> /* snprintf */


/* float */
int seal_format_float(seal_float f, char *buf, int bufsiz)
{
    int n;
    n = snprintf(buf, bufsiz, SEAL_FLOAT_FMT, f);
    if (buf[strspn(buf, "-0123456789")] == '\0') {
        buf[n++] = '.';
        buf[n++] = '0';
        buf[n] = '\0';
    }
    return n;
}

/* string */
const char *string_duplen(const char *s, int len)
{
    if (len < 0)
        len = strlen(s);

    char *dup = SEAL_MALLOC((len + 1) * sizeof(char));
    memcpy(dup, s, len);
    dup[len] = '\0';
    return dup;
}

const char *string_concat(const char *a, const char *b)
{
    int lena = strlen(a), lenb = strlen(b);
    int len = lena + lenb + 1;
    char *res = SEAL_MALLOC(len * sizeof(char));
    memcpy(res, a, lena);
    memcpy(res + lena, b, lenb + 1);
    return res;
}

struct seal_string *string_new(const char *s, bool collect, bool dup)
{
    struct seal_string *str = SEAL_MALLOC(sizeof(struct seal_string));
    
    if (!str)
        return NULL;

    str->collect = collect;
    if (collect)
        str->ref_count = 0;

    str->len = strlen(s);

    str->val = dup ? string_dup(s) : s;

    return str;
}

/* list */
struct seal_list *list_new(int cap)
{
    struct seal_list *list = SEAL_MALLOC(
            sizeof(struct seal_list) + cap * sizeof(struct seal_value)
        );

    if (!list)
        return NULL;

    list->ref_count = 0;
    list->len = 0;
    list->cap = cap;
    list->collect = true;

    return list;
}

/* hashmap */
static unsigned int hashswl(const char *key, int len)
{
    unsigned int hash = 0;
    if (len < 0)
        while (*key)
            hash = *key++ + (hash << 6) + (hash << 16) - hash;
    else
        for (int i = 0; i < len; i++)
            hash = *key++ + (hash << 6) + (hash << 16) - hash;

    return hash;
}

#define hash(k) hashswl(key, -1)

#define hentrynew(k, v) (struct h_entry) { hash(k), (k), strlen(k), (v), true }

struct seal_hashmap *hashmap_new(int cap, bool collect)
{
    struct seal_hashmap *map = SEAL_MALLOC(sizeof(struct seal_hashmap));
    
    if (!map)
        return NULL;

    map->entries = SEAL_CALLOC(cap, sizeof(struct h_entry));
    map->ref_count = 0;
    map->cap = cap;
    map->len = 0;
    map->collect = collect;

    return map;
}

struct h_entry *hashmap_searchlen(struct seal_hashmap *map,
                                  const char *key,
                                  int len)
{
    unsigned int idx = (len < 0 ? hash(key) : hashswl(key, len)) % map->cap;
    struct h_entry *tombstone = NULL;

    struct h_entry *e;
    unsigned int start_idx = idx;
    do {
        e = &map->entries[idx];
        if (e->key == NULL) {
            if (e->is_tomb) {
                if (tombstone == NULL)
                    tombstone = e;
            } else {
                return tombstone != NULL ? tombstone : e;
            }
        } else if (len < 0 ?
                   strcmp(e->key, key) == 0 :
                   e->keysize == len && strncmp(e->key, key, len) == 0) {
            return e;
        }
        idx = (idx + 1) % map->cap;
    } while (idx != start_idx);

    return NULL;
}

static void hashmap_insert_nocheck(struct seal_hashmap *map,
                                   struct h_entry *e)
{
    unsigned int idx = e->hash % map->cap;

    while (map->entries[idx].key != NULL) {
        idx = (idx + 1) % map->cap;
    }

    map->entries[idx] = *e;
    map->len++;
}
/* FIX: OPTIMIZE IT, DO NOT MAKE IT RECURSIVE WHEN RESIZING */
int hashmap_insert(struct seal_hashmap *map,
                   const char *key,
                   struct seal_value val)
{
    if ((float)map->len / (float)map->cap >= HASHMAP_LOAD_FACTOR) {
        int old_cap = map->cap;
        struct h_entry *old_entries = map->entries;

        map->len = 0;
        map->cap *= 2;
        map->entries = SEAL_CALLOC(map->cap, sizeof(struct h_entry));

        for (int i = 0; i < old_cap; i++) {
            struct h_entry *e = &old_entries[i];
            if (e->key)
                hashmap_insert_nocheck(map, e);
        }
        SEAL_FREE(old_entries);
    }

    struct h_entry *searched = hashmap_search(map, key);

    struct h_entry e = hentrynew(key, val);

    bool is_new = searched->key == NULL;
    if (is_new)
        map->len++;

    *searched = e;

    return is_new;
}

int hashmap_insert_e(struct seal_hashmap *map,
                     struct h_entry *entry,
                     const char *key,
                     struct seal_value val)
{
    struct h_entry e = hentrynew(key, val);

    bool is_new = entry->key == NULL;
    if (is_new)
        map->len++;

    *entry = e;

    return is_new;
}

int hashmap_remove(struct seal_hashmap *map, const char *key)
{
  if (map->len <= 0)
      return 1;

  struct h_entry *searched = hashmap_search(map, key);

  if (searched->key == NULL)
    return 1;

  searched->is_tomb = true;
  searched->key = NULL;

  map->len--;

  return 0;
}

int hashmap_free(struct seal_hashmap *map, bool free_key, bool collect)
{
    if (!map)
        return 1;

    for (int i = 0; i < map->cap; i++) {
        struct h_entry e = map->entries[i];
        if (!e.key)
            continue;

        if (free_key)
            SEAL_FREE((char*)e.key);

        if (collect) {}
            /* gc collect seal value */;
    }

    free(map->entries);
    free(map);

    return 0;
}
