#include "seal.h"
#include "state.h"

static void map_has(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TMAP);
    const char *key = seal_checkstring(S, 1);
    struct h_entry *e = hashmap_search(SEAL_AS_MAP(seal_getstack(S, 0)), key);
    bool result = !nullhentry(e);
    seal_pushbool(S, result);
}

static void map_remove(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TMAP);
    const char *key = seal_checkstring(S, 1);
    int status = hashmap_remove(SEAL_AS_MAP(seal_getstack(S, 0)), key);
    if (status) {
        seal_throw(S, "at Map.remove: does not have \'%s\' key", key);
    }
    seal_pushnull(S);
}

static void map_keys(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TMAP);
    struct seal_hashmap *m = SEAL_AS_MAP(seal_getstack(S, 0));
    for (int i = 0; i < m->cap; i++) {
        if (m->entries[i].key) {
            seal_pushstringc(S, m->entries[i].key);
        }
    }
    seal_makelist(S, m->len);
}

static void map_values(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TMAP);
    struct seal_hashmap *m = SEAL_AS_MAP(seal_getstack(S, 0));
    for (int i = 0; i < m->cap; i++) {
        if (m->entries[i].key) {
            seal_push(S, m->entries[i].val);
        }
    }
    seal_makelist(S, m->len);
}

static void map_len(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TMAP);
    seal_pushint(S, SEAL_AS_MAP(seal_getstack(S, 0))->len);
}

#define REG(name) { #name, map_##name }

static const seal_reg maplib[] = {
    REG(has),
    REG(remove),
    REG(keys),
    REG(values),
    REG(len),
    { NULL, NULL }
};

void sealopen_map(seal_state *S)
{
    seal_newlib(S, maplib);
    seal_setglobal(S, "Map");
}
