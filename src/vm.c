#include "vm.h"
#include "compiler.h"
#include <stdio.h>

#define FETCH(S) (*(S)->ip++)
#define GET_CONST(S, i) ( \
        SEAL_AS_FUNC( \
            S->stack[ \
                S->ci->func_idx \
            ] \
        )->as.s.c->pool[i] \
)

#define seal_pushconst(S, i)  seal_push(S, GET_CONST(S, i));

#define IS_FALSY(v) ( \
    SEAL_IS_NULL(v) || ( \
        SEAL_IS_BOOL(v) && !SEAL_AS_BOOL(v) \
    ) \
)

int eval(seal_state *S)
{
    seal_byte op;
    int idx;
    int n;
    signed short jmp_offset;
    call_info *prev_ci;
    struct seal_value popped;

    for (;;) {
        op = FETCH(S);

        switch (op) {
        case OP_HALT:
            return 0;
        case OP_PUSH:
            idx = FETCH(S);
            seal_pushconst(S, idx);
            break;
        case OP_LPUSH:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            seal_pushconst(S, idx);
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
            seal_pushint(S, FETCH(S));
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
            if (SEAL_IS_NULL(popped))
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
        /*
        case OP_ADD:
            break;
        case OP_SUB:
            break;
        case OP_MUL:
            break;
        case OP_DIV:
            break;
        case OP_MOD:
            break;
        case OP_AND:
            break;
        case OP_OR:
            break;
        case OP_XOR:
            break;
        case OP_SHL:
            break;
        case OP_SHR:
            break;
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
            if (seal_getglobal(S, SEAL_AS_STRINGVAL(GET_CONST(S, idx)))) {
                printf("\'%s\' is not defined\n", SEAL_AS_STRINGVAL(GET_CONST(S, idx)));
                return 1;
            }
            break;
        case OP_GETGLOBAL_SAFE:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            if (seal_getglobal(S, SEAL_AS_STRINGVAL(GET_CONST(S, idx)))) {
                seal_pushnull(S);
            }
            break;
        case OP_SETGLOBAL:
            seal_dup(S);
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            seal_setglobal(S, SEAL_AS_STRINGVAL(GET_CONST(S, idx)));
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
            printf("\"%s\": unspecified operation\n", get_opname(op));
            SEAL_ASSERT(0);
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
        printf("%s", SEAL_AS_BOOL(*v) ? "true" : "false");
        break;
    case SEAL_TINT:
        printf("%lld", SEAL_AS_INT(*v));
        break;
    case SEAL_TFLOAT:
        printf("%g", SEAL_AS_FLOAT(*v));
        break;
    case SEAL_TSTRING:
        printf("%s", SEAL_AS_STRINGVAL(*v));
        break;
    case SEAL_TLIST:
        printf("list: %p", (void*)SEAL_AS_LIST(*v));
        break;
    case SEAL_TMAP:
        printf("map: %p", (void*)SEAL_AS_MAP(*v));
        break;
    case SEAL_TFUNCTION:
        printf("function: %p", (void*)SEAL_AS_FUNC(*v));
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
