#include "state.h"
#include "core.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include <stdio.h>


#define GLOBALS_START_SIZE 64
#define STACK_START_SIZE 1024
#define CALL_FRAME_START_SIZE 64

seal_state *seal_state_new()
{
    seal_state *S = SEAL_MALLOC(sizeof(seal_state));

    S->file_name = "<stdin>";

    S->stack =
            SEAL_MALLOC(sizeof(struct seal_value) * STACK_START_SIZE);

    S->sp = 0;

    S->globals = hashmap_Nnew(GLOBALS_START_SIZE);

    S->ci_arr = SEAL_MALLOC(sizeof(struct call_info) * CALL_FRAME_START_SIZE);
    S->ci_idx = -1;
    S->ci = S->ci_arr + S->ci_idx;
    S->ip = NULL;

    S->l.lexemes = NULL;

    // Initialize core if needed
    seal_core_init(S);

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

static void free_chunk(struct chunk *c, int free_cp) {
    SEAL_FREE(c->code);
    SEAL_FREE(c->li);
    for (int i = 0; i < c->pool_size; i++) {
        struct seal_value v = c->pool[i];
        if (SEAL_IS_STRING(v)) {
            if (SEAL_AS_STRING(v)->collect) {
                SEAL_FREE((void*)SEAL_AS_STRING(v)->val);
            }
            SEAL_FREE(v.as.string);
        } else if (SEAL_IS_FUNC(v)) {
            if (SEAL_AS_FUNC(v)->type == FUNCTION_TYPE_SEAL) {
                free_chunk(SEAL_AS_FUNC(v)->as.s.c, true);
            }
            SEAL_FREE(SEAL_AS_FUNC(v));
        }
    }
    SEAL_FREE(c->pool);

    if (free_cp)
        SEAL_FREE(c);
}

int seal_dostring(seal_state *S, const char *str)
{
    lexer_init(&S->l, str);
    struct parser p;
    struct ast *root;
    struct chunk *c = SEAL_MALLOC(sizeof(struct chunk));

    parser_init(&p, &S->l);
    root = parse(&p);
    //dump_ast(root, 0);
    *c = compile(root, NULL);
    //dump_chunk(&c);
    //dump_bytecode(&c);
    arena_free(p.a);

    /* push main function with chunk
     * call it
     * then eval
     */
    struct seal_value fval;
    fval.type = SEAL_TFUNCTION;
    struct seal_func *func = SEAL_MALLOC(sizeof(struct seal_func));
    fval.as.func = func;
    SEAL_AS_FUNC(fval)->type = FUNCTION_TYPE_SEAL;
    SEAL_AS_FUNC(fval)->as.s.name  = "main";
    SEAL_AS_FUNC(fval)->as.s.psize = 0;
    SEAL_AS_FUNC(fval)->as.s.c = c;

    stack_idx start_idx = S->sp;
    seal_push(S, fval);
    seal_call(S, 0);
    if (S->ci_idx == 0) {
        if (eval(S)) { /* fail */
            S->sp = start_idx;
        } else { /* success */
            S->sp--; /* ignore pushed null */
        }
    }


    /* do not free needed functions */
    //free_chunk(&c, false);

    return 0;
}

int seal_dofile(seal_state *S, const char *file_name)
{
    FILE *f = fopen(file_name, "rb");
    SEAL_ASSERT(f != NULL);

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';

    fclose(f);

    const char *prev_file_name = S->file_name;
    S->file_name = file_name;
    int result = seal_dostring(S, src);
    S->file_name = prev_file_name;

    free(src);

    return result;
}

int seal_call(seal_state *S, int argc)
{
    struct seal_value f;
    f = seal_getstack(S, -argc - 1);
    SEAL_ASSERT(SEAL_IS_FUNC(f));
    S->ci++;
    S->ci_idx++;
    if (SEAL_AS_FUNC(f)->type == FUNCTION_TYPE_SEAL) {
        SEAL_ASSERT(SEAL_AS_FUNC(f)->as.s.psize == argc);
    }
    S->ci->func_idx = S->sp - argc - 1;

    if (SEAL_AS_FUNC(f)->type == FUNCTION_TYPE_SEAL) {
        S->ci->ret_ip = S->ip;
        S->ip = SEAL_AS_FUNC(f)->as.s.c->code;
        S->sp += SEAL_AS_FUNC(f)->as.s.c->local_size - argc;
    } else {
        SEAL_AS_FUNC(f)->as.c.f /* function */
            (S); /* calling */

        S->stack[S->ci->func_idx] = seal_getstack(S, -1);
        S->sp = S->ci->func_idx + 1; /* always point to first empty slot */
        S->ci--;
        S->ci_idx--;
    }

    return 0;
}

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
