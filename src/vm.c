#include "vm.h"
#include "compiler.h"
#include <stdio.h>

#define is_null   SEAL_IS_NULL
#define is_bool   SEAL_IS_BOOL
#define is_int    SEAL_IS_INT
#define is_float  SEAL_IS_FLOAT
#define is_str    SEAL_IS_STRING
#define is_list   SEAL_IS_LIST
#define is_map    SEAL_IS_MAP
#define is_func   SEAL_IS_FUNC

#define as_bool   SEAL_AS_BOOL
#define as_int    SEAL_AS_INT
#define as_float  SEAL_AS_FLOAT
#define as_str    SEAL_AS_STRING
#define as_strv   SEAL_AS_STRINGVAL
#define as_list   SEAL_AS_LIST
#define as_map    SEAL_AS_MAP
#define as_func   SEAL_AS_FUNC

static const char *const _type_names[] = {
    [SEAL_TNULL] = "null",
    [SEAL_TBOOL] = "bool",
    [SEAL_TINT] = "integer",
    [SEAL_TFLOAT] = "float",
    [SEAL_TSTRING] = "string",
    [SEAL_TLIST] = "list",
    [SEAL_TMAP] = "map",
    [SEAL_TFUNCTION] = "function"
};

#define valt_name(v) (_type_names[(v).type])

#define error(S, fmt, ...) do { \
    fprintf(stderr, \
            "seal: %s:%d: ", \
            S->file_name,   \
            get_line(CUR_FUNC(S)->as.s.c, S->ip - CUR_FUNC(S)->as.s.c->code)); \
    fprintf(stderr, fmt, __VA_ARGS__); \
    fputc('\n', stderr); \
    return 1; \
} while (0)

#define FETCH(S) (*(S)->ip++)

#define CUR_FUNC(S) as_func((S)->stack[(S)->ci->func_idx])

#define GET_CONST(S, i) (CUR_FUNC(S)->as.s.c->pool[i])

#define PUSH_CONST(S, i)  seal_push(S, GET_CONST(S, i))

#define IS_FALSY(v) ( \
    is_null(v) || ( \
        is_bool(v) && !as_bool(v) \
    ) \
)

#define get_ab(S, a, b) ((b) = seal_pop(S), (a) = seal_pop(S))

/* arithmetic */
#define int_op(S, op, a, b)   seal_pushint(S, as_int(a) op as_int(b))
#define float_op(S, op, a, b) seal_pushfloat(S, as_float(a) op as_float(b))
#define iorf_op(S, op, a, b)  \
    seal_pushfloat(S, \
        (is_int(a) ? as_int(a) : as_float(a)) op \
        (is_int(b) ? as_int(b) : as_float(b)))

#define bin_op_err(S, op, a, b) \
    error(S, \
          "\'%s\' operator does not support \'%s\' and \'%s\'", \
          #op, valt_name(a), valt_name(b)); \

#define bin_op(S, op, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, op, a, b); \
    else if (is_float(a) && is_float(b)) \
        float_op(S, op, a, b); \
    else if ((is_int(a) && is_float(b)) || (is_float(a) && is_int(b))) \
        iorf_op(S, op, a, b); \
    else \
        bin_op_err(S, op, a, b); \
} while (0)

#define mod_op(S, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, %, a, b); \
    else \
        bin_op_err(S, %, a, b); \
} while (0)

#define onlyint_op(S, op, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, op, a, b); \
    else \
        bin_op_err(S, op, a, b); \
} while (0)

int eval(seal_state *S)
{
    seal_byte op;
    int idx;
    int n;
    signed short jmp_offset;
    call_info *prev_ci;
    struct seal_value popped;
    struct seal_value a, b; /* a - left, b - right */

    for (;;) {
        op = FETCH(S);

        switch (op) {
        case OP_HALT:
            return 0;
        case OP_PUSH:
            idx = FETCH(S);
            PUSH_CONST(S, idx);
            break;
        case OP_LPUSH:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            PUSH_CONST(S, idx);
            break;
        case OP_PUSHNULL:
            seal_pushnull(S);
            break;
        case OP_PUSHTRUE:
            seal_pushtrue(S);
            break;
        case OP_PUSHFALSE:
            seal_pushfalse(S);
            break;
        case OP_PUSH8:
            seal_pushint(S, FETCH(S));
            break;
        case OP_PUSH16:
            n  = FETCH(S) << 8;
            n |= FETCH(S);
            seal_pushint(S, n);
            break;
        case OP_POP:
            seal_pop(S);
            break;
        case OP_DUP:
            seal_dup(S);
            break;
        /*
        case OP_COPY:
        case OP_SWAP:
        */
        case OP_JMP:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            S->ip += jmp_offset;
            break;
        case OP_JTRUE:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            popped = seal_pop(S);
            if (!IS_FALSY(popped))
                S->ip += jmp_offset;
            break;
        case OP_JFALSE:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            popped = seal_pop(S);
            if (IS_FALSY(popped))
                S->ip += jmp_offset;
            break;
        case OP_JNULL:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            popped = seal_pop(S);
            if (is_null(popped))
                S->ip += jmp_offset;
            break;
        case OP_CALL:
            seal_call(S, FETCH(S));
            break;
        case OP_RETURN:
            prev_ci = S->ci - 1;

            S->ip = S->ci->ret_ip;
            S->stack[S->ci->func_idx] = seal_getstack(S, -1);
            S->sp = S->ci->func_idx + 1; /* always point to empty slot */
            S->ci = prev_ci;
            S->ci_idx--;
            if (S->ci_idx < 0) {
                return 0;
            }
            break;

        /* binaries */
        case OP_ADD:
            get_ab(S, a, b);
            bin_op(S, +, a, b);
            break;
        case OP_SUB:
            get_ab(S, a, b);
            bin_op(S, -, a, b);
            break;
        case OP_MUL:
            get_ab(S, a, b);
            bin_op(S, *, a, b);
            break;
        case OP_DIV:
            get_ab(S, a, b);
            bin_op(S, /, a, b);
            break;
        case OP_MOD:
            get_ab(S, a, b);
            mod_op(S, a, b);
            break;
        case OP_AND:
            get_ab(S, a, b);
            onlyint_op(S, &, a, b);
            break;
        case OP_OR:
            get_ab(S, a, b);
            onlyint_op(S, |, a, b);
            break;
        case OP_XOR:
            get_ab(S, a, b);
            onlyint_op(S, ^, a, b);
            break;
        case OP_SHL:
            get_ab(S, a, b);
            onlyint_op(S, <<, a, b);
            break;
        case OP_SHR:
            get_ab(S, a, b);
            onlyint_op(S, >>, a, b);
            break;
        /*
        case OP_GT:
            break;
        case OP_GE:
            break;
        case OP_LT:
            break;
        case OP_LE:
            break;
        case OP_EQ:
            break;
        case OP_NE:
            break;
        */
        /* unaries */
        /*
        case OP_NOT:
            break;
        case OP_BNOT:
            break;
        case OP_NEG:
            break;
        */
        /* symbols */
        case OP_GETGLOBAL:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            if (seal_getglobal(S, as_strv(GET_CONST(S, idx)))) {
                error(S, "\'%s\' is not defined\n", as_strv(GET_CONST(S, idx)));
            }

            break;
        case OP_GETGLOBAL_SAFE:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            if (seal_getglobal(S, as_strv(GET_CONST(S, idx)))) {
                seal_pushnull(S);
            }
            break;
        case OP_SETGLOBAL:
            seal_dup(S);
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            seal_setglobal(S, as_strv(GET_CONST(S, idx)));
            break;
        case OP_GETLOCAL:
            idx = FETCH(S);
            seal_push(S, seal_getstack(S, idx));
            break;
        case OP_SETLOCAL:
            seal_dup(S);
            idx = FETCH(S);
            seal_getstack(S, idx) = seal_pop(S);
            break;
        

#if SEAL_DEBUG
        default:
            error(S, "\"%s\": unspecified operation\n", get_opname(op));
#endif
        }
    }

    return 0;
}

#if SEAL_DEBUG
static void print_val(struct seal_value *v)
{
    switch (v->type) {
    case SEAL_TNULL:
        printf("null");
        break;
    case SEAL_TBOOL:
        printf("%s", as_bool(*v) ? "true" : "false");
        break;
    case SEAL_TINT:
        printf("%lld", as_int(*v));
        break;
    case SEAL_TFLOAT:
        printf("%g", as_float(*v));
        break;
    case SEAL_TSTRING:
        printf("%s", as_strv(*v));
        break;
    case SEAL_TLIST:
        printf("list: %p", (void*)as_list(*v));
        break;
    case SEAL_TMAP:
        printf("map: %p", (void*)as_map(*v));
        break;
    case SEAL_TFUNCTION:
        printf("function: %p", (void*)as_func(*v));
        break;
    }
}

void print_stack(seal_state *S)
{
    stack_idx i = 0;
    while (i < S->sp) {
        printf("[%2d]: ", i);
        print_val(S->stack + i);
        putchar('\n');
        i++;
    }
}

#endif
