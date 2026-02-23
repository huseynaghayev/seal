#ifndef STATE_H
#define STATE_H


#include "value.h"
#include "lexer.h"


#define seal_push(S, v) (*(S)->sp++ = (v))
#define seal_dup(S)  (*(S)->sp = *((S)->sp - 1), (S)->sp++)
#define seal_pop(S)  (*(--((S)->sp)))
#define seal_gettop(S)  ((S)->sp - (S)->ci->func - 1)
#define seal_getstack(S, i) (i >= 0 ? (S)->ci->func + i + 1 : (S)->sp + i)

typedef struct call_info {
    struct seal_value *func; /* the called function */
    seal_byte *ret_ip;
    int local_size;
} call_info;

typedef struct seal_state {
    /* pre-runtime state */
    struct lexer l;

    /* runtime */
    struct seal_value *stack;
    struct seal_value *sp;
    struct seal_hashmap *globals;
    struct call_info *ci_arr;
    int ci_idx;
    struct call_info *ci;
    seal_byte *ip;
} seal_state;


seal_state *seal_state_new();
void seal_state_free(seal_state *S);
int seal_evalstr(seal_state *S, const char *str);
int seal_call(seal_state *S, int argc);
/* return 0 if it existed before, 1 if new */
int seal_setglobal(seal_state *S, const char *name); /* value is on top */
/* return 0 if it exists, 1 if not found (nothing is pushed) */
int seal_getglobal(seal_state *S, const char *name); /* push value on top */

/* push */
#define seal_pushnull(S)  seal_push(S, SEAL_VNULL)
#define seal_pushbool(S, b)  seal_push(S, SEAL_VBOOL(b))
#define seal_pushint(S, n)   seal_push(S, SEAL_VINT(n))
#define seal_pushfloat(S, f) seal_push(S, SEAL_VFLOAT(f))

void seal_pushstring(seal_state *S, const char *str);
void seal_pushCfunc(seal_state *S, seal_Cfunction f);


#endif /* STATE_H */
