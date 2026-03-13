#include "vm.h"
#include "compiler.h"
#include <stdio.h>

#define FETCH(S) (*(S)->ip++)
#define PUSH_CONST(S) ( \
    seal_push( \
        S, \
        (S)->ci->func_ix \
    ) \
)

#define IS_FALSY(v) ( \
    SEAL_IS_NULL(v) || ( \
        SEAL_IS_BOOL(v) && !SEAL_AS_BOOL(v) \
    ) \
)

static void push_const(seal_state *S, int i)
{
    seal_push(
        S,
        SEAL_AS_FUNC(
            S->stack[
                S->ci->func_idx
            ]
        )->as.s.c->pool[i]
    );
}

int eval(seal_state *S)
{
    seal_byte op;
    int idx;
    int n;
    signed short jmp_offset;
    call_info *prev_ci;

    for (;;) {
        op = FETCH(S);

        switch (op) {
        case OP_HALT:
            return 0;
        case OP_PUSH:
            idx = FETCH(S);
            push_const(S, idx);
            break;
        case OP_LPUSH:
            idx  = FETCH(S) << 8;
            idx |= FETCH(S);
            push_const(S, idx);
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
            if (!IS_FALSY(seal_pop(S)))
                S->ip += jmp_offset;
            break;
        case OP_JFALSE:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            if (IS_FALSY(seal_pop(S)))
                S->ip += jmp_offset;
            break;
        case OP_JNULL:
            jmp_offset  = FETCH(S) << 8;
            jmp_offset |= FETCH(S);
            if (SEAL_IS_NULL(seal_pop(S)))
                S->ip += jmp_offset;
            break;
        case OP_CALL:
            seal_call(S, FETCH(S));
            break;
        case OP_RETURN:
            prev_ci = S->ci - 1;
            S->ci_idx--;
            if (S->ci_idx < 0) {
                return 0;
            }
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
