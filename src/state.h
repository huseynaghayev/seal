#ifndef STATE_H
#define STATE_H


#include "seal.h"
#include "value.h"
#include "lexer.h"
#include <setjmp.h>

#define SEAL_OK  0
#define SEAL_ERR_LEX    1
#define SEAL_ERR_PARSE  2
#define SEAL_ERR_COMPILE  3
#define SEAL_ERR_RUNTIME  4

#define GC_THRESHOLD 128

#define seal_set_errcode(S, code) ((S)->status = (code))

#define seal_push(S, v) ((S)->stack[(S)->sp++] = (v))
#define seal_dup(S) ( \
    (S)->stack[(S)->sp] = (S)->stack[(S)->sp - 1], \
    (S)->sp++ \
)
#define seal_pop(S)  ((S)->stack[--(S)->sp])
#define seal_getstack(S, i) ((S)->stack[i >= 0 ? (S)->ci->func_idx + i + 1 : (S)->sp + i])

typedef int stack_idx;

typedef struct call_info {
    stack_idx func_idx; /* the called function stack index */
    seal_byte *ret_ip;
    int local_size;
    const char *file_name; /* where the function called from */
    int line; /* and the line info, if 0, it is main chunk */
} call_info;

typedef struct seal_state {
    /* debug info */
    const char *file_name;
    jmp_buf fail_point; /* used for error handling */
    int status;
    int repl_mode;
    char errmsg[SEAL_ERRMSG_BUFSIZ];
    char prev_errmsg[SEAL_ERRMSG_BUFSIZ];
    char stktrc[SEAL_STKTRC_BUFSIZ];
    /* pre-runtime state */
    struct lexer l;

    /* runtime */
    struct seal_value *stack; /* stack array */
    stack_idx sp;    /* stack pointer, always point to first empty slot */
    struct seal_hashmap *globals; /* globals map */
    gc gc;
    struct call_info *ci_arr;     /* call info array */
    int ci_idx;    /* call info index */
    struct call_info *ci; /* current call info */
    seal_byte *ip; /* instruction pointer */
    struct seal_hashmap *packages; /* loaded packages */
    struct seal_hashmap *string_lib;
    struct seal_hashmap *list_lib;
    unsigned int random_state;
} seal_state;


seal_state *seal_state_new();
void seal_state_free(seal_state *S);
void seal_error(seal_state *S, int errln, const char *fmt, ...);
void seal_throw(seal_state *S, const char *msg, ...);
const char *seal_geterror(seal_state *S);

int seal_dostring(seal_state *S, const char *str);
int seal_dofile(seal_state *S, const char *file_name);
int seal_call(seal_state *S, int argc);
int seal_icall(seal_state *S, int argc);

int seal_gettop(seal_state *S);
void seal_movetop(seal_state *S, int offset);
void seal_checkargcopt(seal_state *S, int min, int is_var);
#define seal_checkargc(S, c) seal_checkargcopt(S, c, false)
#define seal_checkargcvar(S, c) seal_checkargcopt(S, c, true)
int seal_gettype(seal_state *S, int i);
const char *seal_gettypename(seal_state *S, int i);

/* values */
seal_bool   seal_tobool(seal_state *S, int i);
seal_int    seal_toint(seal_state *S, int i);
seal_float  seal_tofloat(seal_state *S, int i);
seal_float  seal_tonumber(seal_state *S, int i);
const char *seal_tostring(seal_state *S, int i);

void        seal_checktype(seal_state *S, int i, int type);
seal_bool   seal_checkbool(seal_state *S, int i);
seal_int    seal_checkint(seal_state *S, int i);
seal_float  seal_checkfloat(seal_state *S, int i);
seal_float  seal_checknumber(seal_state *S, int i);
const char *seal_checkstring(seal_state *S, int i);

/* push */

void seal_pushidx(seal_state *S, int i);
void seal_pushnull(seal_state *S);
void seal_pushbool(seal_state *S, int b);
void seal_pushint(seal_state *S, seal_int n);
void seal_pushfloat(seal_state *S, seal_float f);
void seal_pushstring(seal_state *S, const char *str);
void seal_pushCfunc(seal_state *S, seal_Cfunction f);
void seal_makelist(seal_state *S, int size);
#define seal_newlist(S) seal_makelist(S, 0)
void seal_makemap(seal_state *S, int size);
#define seal_newmap(S) seal_makemap(S, 0)

/* get */
/* return 0 if it exists, 1 if not found (nothing is pushed) */
int seal_getglobal(seal_state *S, const char *name); /* push value on top */
int seal_getindex(seal_state *S, int i);
/* return 0 if it exists
 * 1 if not found (null is pushed for now)
 * -1 if not map (null is pushed for now)
 */
int seal_getfield(seal_state *S, int map_i, const char *key);

/* set */
/* return 0 if it existed before, 1 if new */
int seal_setglobal(seal_state *S, const char *name); /* value is on top */
int seal_setindex(seal_state *S, int list_i, int i);
/* return 0 if it existed before
 * 1 if new
 * -1 if not map
 */
int seal_setfield(seal_state *S, int map_i, const char *key);

/* TODO: fix seal.h state.h same function declarations */
/*
typedef struct {
    const char *name;
    seal_Cfunction f;
} seal_reg;

#define seal_register(S, f, name) ( \
    seal_pushCfunc(S, f), \
    seal_setglobal(S, name) \
)
void seal_newlib(seal_state *S, const seal_reg *reg);
*/

void seal_gc(seal_state *S);

#endif /* STATE_H */
