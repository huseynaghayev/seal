#ifndef STATE_H
#define STATE_H


#include "seal.h"
#include "value.h"
#include "lexer.h"
#include "seal.h"
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


void seal_error(seal_state *S, int errln, const char *fmt, ...);
void seal_gc(seal_state *S);

#endif /* STATE_H */
