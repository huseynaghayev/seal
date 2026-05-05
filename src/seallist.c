#include "seal.h"
#include "state.h"

#define IS_FALSY(v) (SEAL_IS_NULL(v) || (SEAL_IS_BOOL(v) && !SEAL_AS_BOOL(v)))

static void list_len(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_pushint(S, SEAL_AS_LIST(seal_getstack(S, 0))->len);
}

static void list_push(seal_state *S)
{
    seal_checkargcvar(S, 2);
    int n = seal_gettop(S);
    seal_checktype(S, 0, SEAL_TLIST);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    for (int i = 1; i < n; i++) {
        list_pushval(l, seal_getstack(S, i));
    }
    seal_pushnull(S);
}

static void list_pop(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TLIST);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    seal_push(S, l->vals[--l->len]);
}

static void list_remove(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TLIST);
    int idx = seal_checkint(S, 1);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    int len = l->len;
    if (idx >= len || (idx < 0 && (idx = len + idx) < 0)) {
        seal_throw(S, "at List.remove: index is out of bounds");
    }
    struct seal_value v = l->vals[idx];
    if (idx == len - 1) {
        seal_push(S, l->vals[--l->len]);
        return;
    }
    for (int i = idx; i < len - 1; i++) {
        l->vals[i] = l->vals[i + 1];
    }
    l->len--;
    seal_push(S, v);
}

static void list_map(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_checktype(S, 1, SEAL_TFUNCTION);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    struct seal_value f = seal_getstack(S, 1);
    for (int i = 0; i < l->len; i++) {
        seal_push(S, f);
        seal_push(S, l->vals[i]);
        seal_icall(S, 1);
    }
    seal_makelist(S, l->len);
}

static void list_filter(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_checktype(S, 1, SEAL_TFUNCTION);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    struct seal_value f = seal_getstack(S, 1);
    int count = 0;
    for (int i = 0; i < l->len; i++) {
        seal_push(S, f);
        seal_push(S, l->vals[i]);
        seal_icall(S, 1);
        struct seal_value r = seal_pop(S);
        if (!IS_FALSY(r)) {
            seal_push(S, l->vals[i]);
            count++;
        }
    }
    seal_makelist(S, count);
}

static void list_reduce(seal_state *S)
{
    seal_checkargc(S, 3);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_checktype(S, 1, SEAL_TFUNCTION);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    struct seal_value f = seal_getstack(S, 1);
    for (int i = 0; i < l->len; i++) {
        seal_push(S, f);
        seal_pushidx(S, 2);
        seal_push(S, l->vals[i]);
        seal_icall(S, 2);
        seal_getstack(S, 2) = seal_pop(S);
    }
    seal_pushidx(S, 2);
}

static void list_any(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_checktype(S, 1, SEAL_TFUNCTION);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    struct seal_value f = seal_getstack(S, 1);
    bool result = false;
    for (int i = 0; i < l->len; i++) {
        seal_push(S, f);
        seal_push(S, l->vals[i]);
        seal_icall(S, 1);
        struct seal_value r = seal_pop(S);
        if (!IS_FALSY(r)) {
            result = true;
            break;
        }
    }
    seal_pushbool(S, result);
}

static void list_all(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_checktype(S, 0, SEAL_TLIST);
    seal_checktype(S, 1, SEAL_TFUNCTION);
    struct seal_list *l = SEAL_AS_LIST(seal_getstack(S, 0));
    struct seal_value f = seal_getstack(S, 1);
    bool result = true;
    for (int i = 0; i < l->len; i++) {
        seal_push(S, f);
        seal_push(S, l->vals[i]);
        seal_icall(S, 1);
        struct seal_value r = seal_pop(S);
        if (IS_FALSY(r)) {
            result = false;
            break;
        }
    }
    seal_pushbool(S, result);
}

#define REG(name) { #name, list_##name }

static const seal_reg listlib[] = {
    REG(len),
    REG(push),
    REG(pop),
    REG(remove),
    REG(map),
    REG(filter),
    REG(reduce),
    REG(any),
    REG(all),
    { NULL, NULL }
};

void sealopen_list(seal_state *S)
{
    seal_newlib(S, listlib);
    seal_setglobal(S, "List");
}
