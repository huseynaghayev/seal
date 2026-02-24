#include "state.h"
#include "core.h"
#include "parser.h"
#include "compiler.h"


#define GLOBALS_START_SIZE 64
#define STACK_START_SIZE 1024
#define CALL_FRAME_START_SIZE 64

seal_state *seal_state_new()
{
    seal_state *S = SEAL_MALLOC(sizeof(seal_state));

    S->stack =
        S->sp =
            SEAL_MALLOC(sizeof(struct seal_value) * STACK_START_SIZE);

    S->globals = hashmap_Nnew(GLOBALS_START_SIZE);

    S->ci_arr = SEAL_MALLOC(sizeof(struct call_info) * CALL_FRAME_START_SIZE);
    S->ci_idx = -1;
    S->ci = NULL;
    S->ip = NULL;

    S->l.lexemes = NULL;

    // Initialize core if needed
    //seal_core_init(S);

    return S;
}

void seal_state_free(seal_state *S)
{
    hashmap_free(S->globals, false, true);
    SEAL_FREE(S->stack);
    hashmap_free(S->l.lexemes, true, true);
    SEAL_FREE(S->ci_arr);
    SEAL_FREE(S);
}

int seal_evalstr(seal_state *S, const char *str)
{
    lexer_init(&S->l, str);
    struct parser p;
    struct ast *root;
    struct chunk c;

    parser_init(&p, &S->l);
    root = parse(&p);
    //dump_ast(root, 0);
    c = compile(root);
    dump_chunk(&c);
    //dump_bytecode(&c);
    arena_free(p.a);

    /* push main function with chunk
     * call it
     * then eval
     */

    SEAL_FREE(c.code);
    SEAL_FREE(c.li);
    for (int i = 0; i < c.pool_size; i++) {
        if (SEAL_IS_STRING(c.pool[i])) {
            if (SEAL_AS_STRING(c.pool[i])->collect) {
                SEAL_FREE((void*)SEAL_AS_STRING(c.pool[i])->val);
            }
            SEAL_FREE(c.pool[i].as.string);
        }
    }
    SEAL_FREE(c.pool);

    return 0;
}

int seal_call(seal_state *S, int argc);

/* return 0 if it existed, 1 if new */
int seal_setglobal(seal_state *S, const char *name)
{
    int is_new = hashmap_insert(S->globals, name, seal_pop(S));
    return is_new;
}

/* return 0 if it exists and push, 1 if not found (nothing is pushed) */
int seal_getglobal(seal_state *S, const char *name)
{
    struct h_entry *e = hashmap_search(S->globals, name);
    if (nullhentry(e)) {
        return 1;
    }

    seal_push(S, e->val);
    return 0;
}

void seal_pushstring(seal_state *S, const char *str);

void seal_pushCfunc(seal_state *S, seal_Cfunction f)
{
    struct seal_func *func = SEAL_CALLOC(1, sizeof(struct seal_func));
    func->type = FUNCTION_TYPE_C;
    func->as.c.f = f;

    seal_push(S, SEAL_VFUNC(func));
}
