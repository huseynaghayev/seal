#include "state.h"
#include "parser.h"
#include "compiler.h"
#include "value.h"
#include "vm.h"
#include "libs.h"
#include <stdio.h>
#include <stdarg.h>


#define GLOBALS_START_SIZE 64
#define CALL_FRAME_START_SIZE 64
#define STACK_START_SIZE (CALL_FRAME_START_SIZE * 10)

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

seal_state *seal_state_new()
{
    seal_state *S = SEAL_MALLOC(sizeof(seal_state));

    S->file_name = "<stdin>";

    S->stack =
            SEAL_MALLOC(sizeof(struct seal_value) * STACK_START_SIZE);

    S->sp = 0;

    S->globals = hashmap_Nnew(GLOBALS_START_SIZE);
    S->packages = hashmap_Nnew(8);

    S->ci_arr = SEAL_MALLOC(sizeof(struct call_info) * CALL_FRAME_START_SIZE);
    S->ci_idx = -1;
    S->ci = S->ci_arr + S->ci_idx;
    S->ip = NULL;

    S->l.lexemes = NULL;
    S->l.S = S;

    S->repl_mode = false;

    sealopen_core(S);

#if SEAL_DEBUG
    sealopen_string(S);
    hashmap_insert(S->packages, "string", SEAL_VBOOL(true));
    seal_getglobal(S, "String");
    S->string_lib = SEAL_AS_MAP(seal_pop(S));

    sealopen_list(S);
    hashmap_insert(S->packages, "list", SEAL_VBOOL(true));
    seal_getglobal(S, "List");
    S->list_lib = SEAL_AS_MAP(seal_pop(S));

    sealopen_map(S);
    hashmap_insert(S->packages, "map", SEAL_VBOOL(true));
#else
    S->string_lib = NULL;
    S->list_lib = NULL;
#endif /* SEAL_DEBUG */

    return S;
}

void seal_state_free(seal_state *S)
{
    hashmap_free(S->globals, false);
    SEAL_FREE(S->stack);
    hashmap_free(S->l.lexemes, true);
    SEAL_FREE(S->ci_arr);
    SEAL_FREE(S);
}

static void seal_errorv(seal_state *S, int errln, const char *fmt, va_list vargs)
{
    int offset = 0;
    const char *file_name;
    if (S->status == SEAL_ERR_RUNTIME)
        file_name = SEAL_AS_SFUNC(S->stack[S->ci->func_idx]).file_name;
    else
        file_name = S->file_name;

    offset += snprintf(S->errmsg + offset, SEAL_ERRMSG_BUFSIZ - offset,
                       "seal: %s:%d: ", file_name, errln);
    offset += vsnprintf(S->errmsg + offset, SEAL_ERRMSG_BUFSIZ - offset, fmt, vargs);
}

void seal_error(seal_state *S, int errln, const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);
    seal_errorv(S, errln, fmt, vargs);
    va_end(vargs);
    longjmp(S->fail_point, 1);
}

void seal_throw(seal_state *S, const char *msg, ...)
{
    va_list vargs;
    va_start(vargs, msg);
    int ci_idx = S->ci_idx;
    struct seal_func *f;
    int valid = false;
    int dec = false;
    while (ci_idx >= 0) {
        f = SEAL_AS_FUNC(S->stack[S->ci_arr[ci_idx].func_idx]);
        if (f->type == FUNCTION_TYPE_SEAL) {
            valid = true;
            break;
        }
        dec = true;
        ci_idx--;
    }
    int line = 0;
    if (valid) {
        if (dec) {
            line = S->ci_arr[ci_idx].line;
        } else {
            line = get_line(f->as.s.c, S->ip /* - 1 */);
        }
    }
    seal_errorv(S, line, msg, vargs);
    va_end(vargs);
    longjmp(S->fail_point, 1);
}

const char *seal_geterror(seal_state *S)
{
    return S->errmsg;
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
                free_chunk(SEAL_AS_SFUNC(v).c, true);
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
    struct parser p;
    p.a = NULL;
    struct chunk *volatile c = NULL;
    struct seal_func *volatile func = NULL;
    int result = 0;

    volatile stack_idx prev_sp = S->sp;
    volatile int prev_ci_idx = S->ci_idx;
    call_info *volatile prev_ci = S->ci;

    jmp_buf saved;
    memcpy(saved, S->fail_point, sizeof(jmp_buf));

    if (setjmp(S->fail_point) == 0) {
        lexer_init(&S->l, str);
        parser_init(&p, &S->l);
        struct ast *root = parse(&p);
        //dump_ast(root, 0);
        c = SEAL_MALLOC(sizeof(struct chunk));
        *c = compile(S, root, NULL, S->file_name);
        //dump_chunk(&c);
        //dump_bytecode(&c);
        arena_free(p.a);
        p.a = NULL;

        /* push main function with chunk
         * call it
         * then eval
         */
        struct seal_value fval;
        fval.type = SEAL_TFUNCTION;
        func = SEAL_MALLOC(sizeof(struct seal_func));
        fval.as.func = func;
        SEAL_AS_FUNC(fval)->type = FUNCTION_TYPE_SEAL;
        SEAL_AS_SFUNC(fval).file_name = S->file_name;
        SEAL_AS_SFUNC(fval).line = 0;
        SEAL_AS_SFUNC(fval).name  = "main chunk";
        SEAL_AS_SFUNC(fval).psize = 0;
        SEAL_AS_SFUNC(fval).c = c;

        seal_push(S, fval);
        seal_call(S, 0);
        if (S->ci_idx == 0) {
            if (eval(S, S->ci_idx)) { /* fail */
                S->sp = prev_sp;
                S->ci_idx = prev_ci_idx;
                S->ci = prev_ci;
                // free c
                // free func
                result = 1;
            } else { /* success */
                S->sp--; /* ignore pushed null */
            }
        }
    } else {
        if (p.a)
            arena_free(p.a);
        if (c)
            // free c
            ;
        if (func)
            // free func
            ;
        S->sp = prev_sp;
        S->ci_idx = prev_ci_idx;
        S->ci = prev_ci;
        result = -1;
    }

    /* do not free needed functions */
    //free_chunk(&c, false);

    memcpy(S->fail_point, saved, sizeof(jmp_buf));

    return result;
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
    if (!SEAL_IS_FUNC(f)) {
        seal_throw(S, "cannot call \'%s\' object", _type_names[f.type]);
        return 1;
    }

    if (S->ci_idx >= 0) {
        struct seal_func *prev_f = SEAL_AS_FUNC(S->stack[S->ci->func_idx]);
        if (prev_f->type == FUNCTION_TYPE_SEAL) {
            struct chunk *prev_chunk = SEAL_AS_SFUNC(S->stack[S->ci->func_idx]).c;
            S->ci->line = get_line(prev_chunk, S->ip);
        }
    }

    S->ci++;
    S->ci_idx++;
    S->ci->func_idx = S->sp - argc - 1;

    if (SEAL_AS_FUNC(f)->type == FUNCTION_TYPE_SEAL) {
        /* TODO: after adding variadic arguments into Seal,
         * check for them too
         */
        seal_checkargc(S, SEAL_AS_SFUNC(f).psize);
    }

    if (SEAL_AS_FUNC(f)->type == FUNCTION_TYPE_SEAL) {
        S->ci->file_name = SEAL_AS_SFUNC(f).file_name;
        S->ci->line = 0;
        S->ci->ret_ip = S->ip;
        S->ip = SEAL_AS_SFUNC(f).c->code;
        S->sp += SEAL_AS_SFUNC(f).c->local_size - argc;
    } else {
        SEAL_AS_CFUNC(f).f /* function */
            (S); /* calling */

        S->stack[S->ci->func_idx] = seal_getstack(S, -1);
        S->sp = S->ci->func_idx + 1; /* always point to first empty slot */
        S->ci--;
        S->ci_idx--;
    }

    return 0;
}

int seal_icall(seal_state *S, int argc)
{
    int ci_idx = S->ci_idx;
    int status = seal_call(S, argc);
    if (status || ci_idx == S->ci_idx)
        return status;

    return eval(S, S->ci_idx);
}

int seal_gettop(seal_state *S)
{
    return S->sp - S->ci->func_idx - 1;
}

void seal_movetop(seal_state *S, int offset)
{
    S->sp += offset;
}

void seal_checkargcopt(seal_state *S, int min, int is_var)
{
    int n = seal_gettop(S);
    if (n != min) {
        if (!is_var || (is_var && n < min)) {
            seal_throw(S, "expected %s%d argument%s, got %d",
                       is_var ? "at least " : "", min, min == 1 ? "" : "s", n);
        }
    }
}

int seal_gettype(seal_state *S, int i)
{
    return seal_getstack(S, i).type;
}

const char *seal_gettypename(seal_state *S, int i)
{
    return _type_names[seal_getstack(S, i).type];
}

/* values */
seal_bool seal_tobool(seal_state *S, int i)
{
    return SEAL_AS_BOOL(seal_getstack(S, i));
}

seal_int seal_toint(seal_state *S, int i)
{
    return SEAL_AS_INT(seal_getstack(S, i));
}

seal_float seal_tofloat(seal_state *S, int i)
{
    return SEAL_AS_FLOAT(seal_getstack(S, i));
}

seal_float seal_tonumber(seal_state *S, int i)
{
    struct seal_value v = seal_getstack(S, i);
    return SEAL_AS_NUM(v);
}

const char *seal_tostring(seal_state *S, int i)
{
    return SEAL_AS_STRINGVAL(seal_getstack(S, i));
}

#define val_is(S, i, t, v, out) (v = seal_getstack(S, i), (out = v.type) == (t))

#define checkval_err(S, i, exp, got) \
    seal_throw(S, "argument #%d: expected \'%s\', got \'%s\'", \
               i + 1, _type_names[exp], _type_names[got])

#define check_body(type) \
    struct seal_value v; \
    int out; \
    if (!val_is(S, i, SEAL_T##type, v, out)) \
        checkval_err(S, i, SEAL_T##type, out); \
    return SEAL_AS_##type(v);

void seal_checktype(seal_state *S, int i, int type)
{
    struct seal_value v;
    int out;
    if (!val_is(S, i, type, v, out))
        checkval_err(S, i, type, out);
}

seal_bool seal_checkbool(seal_state *S, int i)
{
    check_body(BOOL);
}

seal_int seal_checkint(seal_state *S, int i)
{
    check_body(INT);
}

seal_float seal_checkfloat(seal_state *S, int i)
{
    check_body(FLOAT);
}

seal_float seal_checknumber(seal_state *S, int i)
{
    struct seal_value v;
    int out;
    if (!val_is(S, i, SEAL_TINT, v, out)) {
        if (out != SEAL_TFLOAT) {
            seal_throw(S, "argument #%d: expected \'%s\', got \'%s\'",
                       i + 1, "number", _type_names[out]);
        }
    }

    return SEAL_AS_NUM(v);
}

const char *seal_checkstring(seal_state *S, int i)
{
    struct seal_value v;
    int out;
    if (!val_is(S, i, SEAL_TSTRING, v, out)) {
        checkval_err(S, i, SEAL_TSTRING, out);
    }

    return SEAL_AS_STRINGVAL(v);
}

/* push */

void seal_pushidx(seal_state *S, int i)
{
    seal_push(S, seal_getstack(S, i));
}

void seal_pushnull(seal_state *S)
{
    seal_push(S, SEAL_VNULL);
}

void seal_pushbool(seal_state *S, int b)
{
    seal_push(S, SEAL_VBOOL(b));
}

void seal_pushint(seal_state *S, seal_int n)
{
    seal_push(S, SEAL_VINT(n));
}

void seal_pushfloat(seal_state *S, seal_float f)
{
    seal_push(S, SEAL_VFLOAT(f));
}

void seal_pushstring(seal_state *S, const char *str)
{
    struct seal_string *s = string_new(str, false, false);
    seal_push(S, SEAL_VSTRING(s));
}

void seal_pushCfunc(seal_state *S, seal_Cfunction f)
{
    struct seal_func *func = SEAL_CALLOC(1, sizeof(struct seal_func));
    func->type = FUNCTION_TYPE_C;
    func->as.c.f = f;

    seal_push(S, SEAL_VFUNC(func));
}

#define ofpow2(n) ( \
    (n)--, \
    (n) |= (n) >> 1, \
    (n) |= (n) >> 2, \
    (n) |= (n) >> 4, \
    (n) |= (n) >> 8, \
    (n) |= (n) >> 16, \
    (n)++ \
)

void seal_makelist(seal_state *S, int size)
{
    if (size == 0) {
        seal_push(S, SEAL_VLIST(list_new(0)));
    } else {
        int n = size;
        ofpow2(n);
        struct seal_list *l = list_new(n);
        struct seal_value *base = &seal_getstack(S, -size);
        for (int i = 0; i < size; i++) {
            list_pushval(l, *base++);
        }
        S->sp -= size;
        seal_push(S, SEAL_VLIST(l));
    }
}

void seal_makemap(seal_state *S, int size)
{
    if (size == 0) {
        seal_push(S, SEAL_VMAP(hashmap_Cnew(8)));
    } else {
        /* TODO: fix hashmap size */
        struct seal_value m = SEAL_VMAP(hashmap_Cnew(size / HASHMAP_LOAD_FACTOR));
        struct seal_value *base = &seal_getstack(S, -size * 2);
        struct seal_value v;
        const char *key;
        for (int i = 0; i < size; i++) {
            key = SEAL_AS_STRINGVAL(*base++);
            v = *base++;
            hashmap_insert(SEAL_AS_MAP(m), key, v);
        }
        S->sp -= size * 2;
        seal_push(S, m);
    }
}

/* get */
/* return 0 if it exists, 1 if not found (nothing is pushed) */
int seal_getglobal(seal_state *S, const char *name) /* push value on top */
{
    struct h_entry *e = hashmap_search(S->globals, name);
    if (nullhentry(e)) {
        return 1;
    }

    seal_push(S, e->val);
    return 0;
}

int seal_getindex(seal_state *S, int i);

/* return 0 if it exists
 * 1 if not found (null is pushed for now)
 * -1 if not map (null is pushed for now)
 */
int seal_getfield(seal_state *S, int map_i, const char *key)
{
    struct seal_value m = seal_getstack(S, map_i);
    if (!SEAL_IS_MAP(m)) {
        /* TODO: throw error when it is not map */
        seal_pushnull(S);
        return 1;
    }
    struct h_entry *e;
    e = hashmap_search(SEAL_AS_MAP(m), key);
    if (nullhentry(e)) {
        /* TODO: throw error when key is not found
         * but return null for now
         */
        seal_pushnull(S);
        return 1;
    }
    seal_push(S, e->val);
    return 0;
}

/* set */
/* return 0 if it existed before, 1 if new */
int seal_setglobal(seal_state *S, const char *name) /* value is on top */
{
    int is_new = hashmap_insert(S->globals, name, seal_pop(S));
    return is_new;
}

int seal_setindex(seal_state *S, int list_i, int i);

/* return 0 if it existed before
 * 1 if new
 * -1 if not map
 */
int seal_setfield(seal_state *S, int map_i, const char *key)
{
    struct seal_value m = seal_getstack(S, map_i);
    if (!SEAL_IS_MAP(m)) {
        /* TODO: throw error when it is not map */
        return -1;
    }
    int is_new = hashmap_insert(SEAL_AS_MAP(m), key, seal_pop(S));
    return is_new;
}

void seal_newlib(seal_state *S, const seal_reg *reg)
{
    seal_newmap(S);
    while (reg->name) {
        seal_pushCfunc(S, reg->f);
        SEAL_AS_CFUNC(S->stack[S->sp - 1]).name = reg->name;
     /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      * TODO: make this better */
        seal_setfield(S, -2, reg->name);
        reg++;
    }
}
