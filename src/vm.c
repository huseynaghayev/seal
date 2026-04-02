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
#define is_num    SEAL_IS_NUM

#define as_bool   SEAL_AS_BOOL
#define as_int    SEAL_AS_INT
#define as_float  SEAL_AS_FLOAT
#define as_str    SEAL_AS_STRING
#define as_strv   SEAL_AS_STRINGVAL
#define as_list   SEAL_AS_LIST
#define as_map    SEAL_AS_MAP
#define as_func   SEAL_AS_FUNC
#define as_sfunc  SEAL_AS_SFUNC
#define as_cfunc  SEAL_AS_CFUNC
#define as_num    SEAL_AS_NUM

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

#define vtype(v) ((v).type)
#define valt_name(v) (_type_names[vtype(v)])

#define FETCH(S) (*(S)->ip++)

#define CUR_SFUNC(S) as_sfunc((S)->stack[(S)->ci->func_idx])

#define GET_CONST(S, i) (CUR_SFUNC(S).c->pool[i])

#define PUSH_CONST(S, i)  seal_push(S, GET_CONST(S, i))

#define IS_FALSY(v) (is_null(v) || (is_bool(v) && !as_bool(v)))

#define get_ab(S, a, b) ((b) = seal_pop(S), (a) = seal_pop(S))

#define vm_error(S, msg, ...) ( \
    seal_set_errcode(S, SEAL_ERR_RUNTIME), \
    store_callstack(S), \
    seal_throw(S, msg, __VA_ARGS__) \
)

#define bin_op_err(S, op, a, b) \
    vm_error(S, \
             "\'%s\' operator does not support \'%s\' and \'%s\'", \
             #op, valt_name(a), valt_name(b)) \

#define unry_op_err(S, op, v) \
    vm_error(S, \
             "\'%s\' operator does not support \'%s\'", \
             #op, valt_name(v)) \

/* arithmetic */
#define int_op(S, op, a, b)   seal_pushint(S, as_int(a) op as_int(b))
#define num_op(S, op, a, b)   seal_pushfloat(S, as_num(a) op as_num(b))

#define bin_op(S, op, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, op, a, b); \
    else if (is_num(a) && is_num(b)) \
        num_op(S, op, a, b); \
    else \
        bin_op_err(S, op, a, b); \
} while (0)

#define mod_op(S, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, %, a, b); \
    else \
        bin_op_err(S, %, a, b); \
} while (0)

/* bitwise */
#define onlyint_op(S, op, a, b) do { \
    if (is_int(a) && is_int(b)) \
        int_op(S, op, a, b); \
    else \
        bin_op_err(S, op, a, b); \
} while (0)

/* comparison */
#define cmp_op(S, op, a, b) do { \
    if (is_num(a) && is_num(b)) \
        seal_pushbool(S, as_num(a) op as_num(b)); \
    else if (is_str(a) && is_str(b)) \
        seal_pushbool(S, strcmp(as_strv(a), as_strv(b)) op 0); \
    else \
        bin_op_err(S, op, a, b); \
} while (0)

/* equality */
#define eql_op(S, op, a, b) do { \
    if (is_num(a) && is_num(b)) \
        seal_pushbool(S, as_num(a) op as_num(b)); \
    else if (is_str(a) && is_str(b)) \
        seal_pushbool(S, strcmp(as_strv(a), as_strv(b)) op 0); \
    else if (is_bool(a) && is_bool(b)) \
        seal_pushbool(S, as_bool(a) op as_bool(b)); \
    else if (is_list(a) && is_list(b)) \
        seal_pushbool(S, (void *)as_list(a) op (void *)as_list(b)); \
    else if (is_map(a) && is_map(b)) \
        seal_pushbool(S, (void *)as_map(a) op (void *)as_map(b)); \
    else if (is_func(a) && is_func(b)) \
        seal_pushbool(S, (void *)as_func(a) op (void *)as_func(b)); \
    else \
        seal_pushbool(S, vtype(a) op vtype(b)); \
} while (0)

/* unaries */
#define bnot_op(S, v) do { \
    if (is_int(v)) \
        seal_pushint(S, ~ as_int(v)); \
    else \
        unry_op_err(S, ~, v); \
} while (0)

#define neg_op(S, v) do { \
    if (is_int(v)) \
        seal_pushint(S, - as_int(v)); \
    else if (is_float(v)) \
        seal_pushfloat(S, - as_float(v)); \
    else \
        unry_op_err(S, -, v); \
} while (0)

static void print_ci(seal_state *S, call_info *ci, int *offset)
{
    struct seal_value f = S->stack[ci->func_idx];

    if (as_func(f)->type == FUNCTION_TYPE_SEAL) {
        if (ci->line > 0)
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "%s:%d: in ", ci->file_name, ci->line);
        else
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "%s: in ", ci->file_name);

        if (as_sfunc(f).line == 0) {
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "main chunk");
        } else if (as_sfunc(f).name) {
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "\'%s\'", as_sfunc(f).name);
        } else {
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "function <%s:%d>", as_sfunc(f).file_name, as_sfunc(f).line);
        }
    } else {
        const char *name = as_cfunc(f).name;
        if (name)
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "\'%s\'", name);
        else
            *offset += snprintf(S->stktrc + *offset, SEAL_STKTRC_BUFSIZ - *offset,
                       "?");
    }
}

static void store_callstack(seal_state *S)
{
    int offset = 0;
    offset += snprintf(S->stktrc + offset, SEAL_STKTRC_BUFSIZ - offset,
              "call stack traceback:\n");
    int i = S->ci_idx;
    S->ci->line = get_line(CUR_SFUNC(S).c, S->ip /* - 1 */);
    while (i >= 0) {
        offset += snprintf(S->stktrc + offset, SEAL_STKTRC_BUFSIZ - offset,
                  "\t");
        print_ci(S, &S->ci_arr[i], &offset);
        i--;
        if (i >= 0) {
            offset += snprintf(S->stktrc + offset, SEAL_STKTRC_BUFSIZ - offset,
                      "\n");
        }
    }
}

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
        case OP_GT:
            get_ab(S, a, b);
            cmp_op(S, >, a, b);
            break;
        case OP_GE:
            get_ab(S, a, b);
            cmp_op(S, >=, a, b);
            break;
        case OP_LT:
            get_ab(S, a, b);
            cmp_op(S, <, a, b);
            break;
        case OP_LE:
            get_ab(S, a, b);
            cmp_op(S, <=, a, b);
            break;
        case OP_EQ:
            get_ab(S, a, b);
            eql_op(S, ==, a, b);
            break;
        case OP_NE:
            get_ab(S, a, b);
            eql_op(S, !=, a, b);
            break;
        /* unaries */
        case OP_NOT:
            popped = seal_pop(S);
            seal_pushbool(S, IS_FALSY(popped) ? 1 : 0);
            break;
        case OP_BNOT:
            popped = seal_pop(S);
            bnot_op(S, popped);
            break;
        case OP_NEG:
            popped = seal_pop(S);
            neg_op(S, popped);
            break;
        /* symbols */
        case OP_GETGLOBAL:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            if (seal_getglobal(S, as_strv(GET_CONST(S, idx)))) {
                vm_error(S, "\'%s\' is not defined", as_strv(GET_CONST(S, idx)));
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

        /* lists and maps */
        /*
        case OP_NEWLIST:
            break;
        case OP_MAKELIST:
            break;
        case OP_PUSHLIST:
            break;
        */
        case OP_NEWMAP:
            seal_pushnewmap(S);
            break;
        /*
        case OP_MAKEMAP:
            break;
        case OP_PUSHMAP:
            break;
        */
        case OP_GETFIELD:
        {
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            const char *key = as_strv(GET_CONST(S, idx));
            if (!is_map(seal_getstack(S, -1))) {
                vm_error(S, "cannot index \'%s\'", valt_name(seal_getstack(S, -1)));
            }
            int status = seal_getfield(S, -1, key);
            if (status == 1) { /* not found */
                vm_error(S, "does not have \'%s\' key", key);
            }

            /* replace map with value */
            seal_getstack(S, -2) = seal_getstack(S, -1);
            /* pop value */
            seal_pop(S);
            break;
        }
        /*
        case OP_GETFIELD_SAFE:
            break;
        */
        case OP_SETFIELD:
        {
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            const char *key = as_strv(GET_CONST(S, idx));
            struct seal_value v = seal_getstack(S, -1);
            if (!is_map(seal_getstack(S, -2))) {
                vm_error(S, "cannot index \'%s\'", valt_name(seal_getstack(S, -2)));
            }
            int status = seal_setfield(S, -2, key);
            seal_getstack(S, -1) = v; /* replace map with value */
            break;
        }
        /*
        case OP_GETINDEX:
            break;
        case OP_SETINDEX:
            break;
        */

         /* other */
        case OP_INCLUDE:
        {
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            const char *file_name = as_strv(GET_CONST(S, idx));
            if (seal_dofile(S, file_name)) { /* throw error */
                strncpy(S->prev_errmsg, S->errmsg, SEAL_ERRMSG_BUFSIZ);
                vm_error(S, "could not include \'%s\' file:\n\t%s", file_name, S->prev_errmsg);
            }
            break;
        }
#if SEAL_DEBUG
        default:
            vm_error(S, "\"%s\": unspecified operation", get_opname(op));
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
