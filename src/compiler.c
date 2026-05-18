#include "compiler.h"
#include "value.h"
#include <stdio.h> /* dump_chunk */

/* constants */
#define CODE_START_CAP 16
#define POOL_START_CAP 8
#define LINE_INFO_START_CAP 4

/* typedefs */
typedef struct chunk chunk;
typedef struct ast ast;
typedef struct seal_value value;
typedef struct h_entry h_entry;

#define MAX_LOOP_DEPTH (SEAL_MAX_INDENT_LEVEL + 1) /* 1 for extra inline loop */

typedef struct {
    seal_byte *code; /* instructions */
    int code_size; /* instructions size */
    int code_cap;  /* instructions capacity */
    value *pool;   /* constant pool */
    int pool_size; /* constant size */
    int pool_cap;  /* constant capacity */
    line_info *li; /* line infos */
    int li_size; /* line info size */
    int li_cap;  /* line info capacity */
    int last_seen_line; /* for constant node values */
    struct {
        int begin_pos;
        signed short last_stop;
    } loops[MAX_LOOP_DEPTH];
    int loop_count;
    const char *file_name;
} proto;

#define push_loop(p, begin) ( \
    (p)->loops[(p)->loop_count++].begin_pos = (begin) \
)

#define pop_loop(p) ( \
    (p)->loops[--(p)->loop_count].last_stop = 0 \
)

#define cur_loop(p) ((p)->loops[(p)->loop_count - 1])

typedef struct {
    struct seal_hashmap *h; /* current scope */
    //struct seal_hashmap *p; /* parent scope for upvalues (linked list) */
} scope;

/* forward declarations */
static void compile_node(proto *p, ast *n, scope *s);

/* macros */
#define tnode(n) ((n)->type)
#define emitn(p, b) (emit(p, b, NULL))
#define emit16(p, i, n) ( \
    emit(p, (i) >> 8, n), \
    emit(p, (i), n) \
)
#define emit16dummy(p) emit16(p, 0, NULL)
#define fit1byte(n) ((n) <= 0xFF)
#define fit2byte(n) ((n) <= 0xFFFF)
#define emitconstidx(p, i, n) do { \
    int _idx = i; \
    if (fit1byte(_idx)) { \
        emit(p, OP_PUSH, n); \
        emit(p, _idx, n); \
    } else if (fit2byte(_idx)) { \
        emit(p, OP_LPUSH, n); \
        emit16(p, _idx, n); \
    } else { \
        SEAL_ASSERT(0 && "out of index: bigger than 0xFFFF"); \
    } \
} while (0)

#define emitconst(p, v, n) do { \
    store(p, v); \
    emitconstidx(p, (p)->pool_size - 1, n); \
} while (0)

#define jmpreplace16(p, a, b) do { \
    SEAL_ASSERT( \
        (a) - ((b) + 2) <= SHRT_MAX && \
        (a) - ((b) + 2) >= SHRT_MIN \
    ); \
    signed short __i = (a) - ((b) + 2); \
    (p)->code[b] = __i >> 8; \
    (p)->code[(b) + 1] = __i;  \
} while (0)

#define jmpreplace16cur(p, pos) jmpreplace16(p, (p)->code_size, pos)

static void backpatch_stops(proto *p)
{
    signed short cur = cur_loop(p).last_stop;
    signed short prev;
    while (cur != 0) {
        int i = cur + cur_loop(p).begin_pos;
        prev = p->code[i] << 8;
        prev |= p->code[i + 1];
        jmpreplace16(p, p->code_size, i);
        cur = prev;
    }
}

static void emit(proto *p, seal_byte b, ast *n)
{
    if (!p->code || p->code_cap == 0) {
        p->code_size = 0;
        p->code_cap = CODE_START_CAP;
        p->code = SEAL_MALLOC(p->code_cap * sizeof(seal_byte));
    } else if (p->code_size >= p->code_cap) {
        p->code_cap *= 2;
        p->code = SEAL_REALLOC(p->code, p->code_cap * sizeof(seal_byte));
    }

    p->code[p->code_size++] = b;
    
    int actual_line = p->last_seen_line;
    if (n && n->line >= 1)
        actual_line = p->last_seen_line = n->line;

    if (!p->li || p->li_cap == 0) {
        p->li_size = 0;
        p->li_cap = LINE_INFO_START_CAP;
        p->li = SEAL_MALLOC(p->li_cap * sizeof(line_info));
    } else if (p->li_size >= p->li_cap) {
        p->li_cap *= 2;
        p->li = SEAL_REALLOC(p->li, p->li_cap * sizeof(line_info));
    }

    if (p->li_size == 0 || actual_line != p->li[p->li_size - 1].line) {
        p->li[p->li_size].line = actual_line;
        p->li[p->li_size].offset = p->code_size - 1;
        p->li_size++;
    }
}

static void store(proto *p, value v)
{
    if (!p->pool || p->pool_cap == 0) {
        p->pool_size = 0;
        p->pool_cap = POOL_START_CAP;
        p->pool = SEAL_MALLOC(p->pool_cap * sizeof(value));
    }
    if (p->pool_size >= p->pool_cap) {
        p->pool_cap *= 2;
        p->pool = SEAL_REALLOC(p->pool, p->pool_cap * sizeof(value));
    }
    p->pool[p->pool_size++] = v;
}

static inline struct seal_string *alloc_seal_string(const char *s)
{
    return string_new(s, false, NULL);
}

static int search_str_inpool(proto *p, const char *s)
{
    for (int i = p->pool_size - 1; i >= 0; i--) {
        /* compare by address since string interning happens in lexing phase */
        if (p->pool[i].type == SEAL_TSTRING && s == p->pool[i].as.string->val) {
            return i;
        }
    }

    return -1;
}

#define pushstringidx(p, s, n) do { \
    int i = search_str_inpool(p, s); \
    if (i != -1) \
        emitconstidx(p, i, n); \
    else \
        emitconst(p, SEAL_VSTRING(alloc_seal_string(s)), n); \
} while (0)

static int get_string_idx(proto *p, const char *s)
{
    int i = search_str_inpool(p, s);
    if (i == -1) {
        store(p, SEAL_VSTRING(alloc_seal_string(s)));
        return p->pool_size - 1;
    }
    return i;
}

static void compile_value(proto *p, ast *n)
{
    switch (tnode(n)) {
    case AST_NULL:
        emit(p, OP_PUSHNULL, n);
        break;
    case AST_TRUE:
        emit(p, OP_PUSHTRUE, n);
        break;
    case AST_FALSE:
        emit(p, OP_PUSHFALSE, n);
        break;
    case AST_INT:
        if (fit1byte(n->as.i)) {
            emit(p, OP_PUSH8, n);
            emit(p, n->as.i, n);
        } else if (fit2byte(n->as.i)) {
            emit(p, OP_PUSH16, n);
            emit16(p, n->as.i, n);
        } else {
            emitconst(p, SEAL_VINT(n->as.i), n);
        }
        break;
    case AST_FLOAT:
        emitconst(p, SEAL_VFLOAT(n->as.f), n);
        break;
    case AST_STRING:
        pushstringidx(p, n->as.s, n);
        break;
    default:
        SEAL_ASSERT(0);
        break;
    }
}

static void compile_list(proto *p, ast *n, scope *s)
{
    int size = n->as.l.size;
    if (size == 0) {
        emit(p, OP_NEWLIST, n);
    } else if (size <= 0xFF) {
        ast *l = n->as.l.items;
        while (l) {
            compile_node(p, l, s);
            l = l->next;
        }
        emit(p, OP_MAKELIST, n);
        emit(p, size, n);
    } else {
        emit(p, OP_NEWLIST, n);
        ast *l = n->as.l.items;
        while (size > 0) {
            int batch = size >= 0xFF ? 0xFF : size;
            for (int i = 0; i < batch; i++) {
                compile_node(p, l, s);
                l = l->next;
            }
            emit(p, OP_PUSHLIST, n);
            emit(p, batch, n);
            size -= batch;
        }
    }
}

static void compile_map(proto *p, ast *n, scope *s)
{
    int size = n->as.m.size;
    if (size == 0) {
        emit(p, OP_NEWMAP, n);
    } else if (size <= 0xFF) {
        ast *m = n->as.m.pairs;
        while (m) {
            pushstringidx(p, m->as.pair.key, m);
            compile_node(p,  m->as.pair.val, s);
            m = m->next;
        }
        emit(p, OP_MAKEMAP, n);
        emit(p, size, n);
    } else {
        emit(p, OP_NEWMAP, n);
        ast *m = n->as.m.pairs;
        while (size > 0) {
            int batch = size >= 0xFF ? 0xFF : size;
            for (int i = 0; i < batch; i++) {
                pushstringidx(p, m->as.pair.key, m);
                compile_node(p, m->as.pair.val, s);
                m = m->next;
            }
            emit(p, OP_PUSHMAP, n);
            emit(p, batch, n);
            size -= batch;
        }
    }
}

static void compile_name(proto *p, ast *n, scope *s)
{
    //if (n->as.name.safe)
    //    SEAL_ASSERT(0 && "? is not compiled");

    if (n->as.name.global) {
        int i = get_string_idx(p, n->as.name.s);
        if (n->as.name.safe)
            emit(p, OP_GETGLOBAL_SAFE, n);
        else
            emit(p, OP_GETGLOBAL, n);

        emit16(p, i, n);
    } else {
        h_entry *e = hashmap_search(s->h, n->as.name.s);
        if (nullhentry(e)) {
            /* not found, search globally at runtime */
            int i = get_string_idx(p, n->as.name.s);
            if (n->as.name.safe)
                emit(p, OP_GETGLOBAL_SAFE, n);
            else
                emit(p, OP_GETGLOBAL, n);

            emit16(p, i, n);
        } else {
            /* found locally */
            emit(p, OP_GETLOCAL, n);
            emit(p, e->val.as.integer, n);
        }
    }
}

static void compile_func_def(proto *p, ast *n, scope *s)
{
    scope local_scope = {0};
    local_scope.h = hashmap_Nnew(SEAL_LOCAL_MAX);
    ast *param = n->as.func.params;
    while (param) {
        int idx = local_scope.h->len;
        hashmap_insert(local_scope.h, param->as.name.s, SEAL_VINT(idx));
        param = param->next;
    }

    struct chunk c = compile(NULL, n->as.func.body, local_scope.h, p->file_name);
    struct chunk *pc = SEAL_MALLOC(sizeof(c));
    *pc = c;
    struct seal_value f;
    f.type = SEAL_TFUNCTION;
    f.as.func = SEAL_MALLOC(sizeof(struct seal_func));
    f.as.func->type = FUNCTION_TYPE_SEAL;
    f.as.func->as.s.c = pc;
    f.as.func->as.s.file_name = p->file_name;
    f.as.func->as.s.line = n->line;
    f.as.func->as.s.name  = n->as.func.name;
    f.as.func->as.s.psize = n->as.func.psize;

    emitconst(p, f, n);
    if (n->as.func.name) {
        if (n->as.func.global) {
            int i = get_string_idx(p, n->as.func.name);
            emit(p, OP_SETGLOBAL, n);
            emit16(p, i, n);
        } else {
            h_entry *e = hashmap_search(s->h, n->as.func.name);
            if (e == NULL) {
                /* HASHMAP IS FULL */
                SEAL_ASSERT(0 && "maximum amount of locals is 256");
            }
            if (e->key == NULL) {
                /* VARIABLE IS NOT SET YET */
                int idx = s->h->len;
                hashmap_insert_e(s->h, e, n->as.func.name, SEAL_VINT(idx));
            }

            emit(p, OP_SETLOCAL, n);
            emit(p, e->val.as.integer, n);
        }
    }
}

static void compile_call(proto *p, ast *n, scope *s)
{
    //SEAL_ASSERT(n->type == AST_FUNC_CALL);
    // TODO: add guard for method (254)
    SEAL_ASSERT(n->as.call.argc <= 0xFF);
    int is_method = (tnode(n) == AST_METH_CALL);
    if (is_method) {
        ast *obj = n->as.call.f;
        compile_node(p, obj->as.field.m, s);
        emitn(p, OP_DUP);
        emit(p, OP_GETFIELD, obj->as.field.f);
        int i = get_string_idx(p, obj->as.field.f->as.name.s);
        emit16(p, i, obj->as.field.f);
        emitn(p, OP_SWAP);
        emitn(p, 1);
    } else {
        compile_node(p, n->as.call.f, s); /* function */
    }

    ast *arg = n->as.call.argv;
    while (arg) { /* compile arguments */
        compile_node(p, arg, s);
        arg = arg->next;
    }

    emit(p, OP_CALL, n); /* call instruction */
    emit(p, n->as.call.argc + is_method, n); /* number of arguments */
}

static void compile_field(proto *p, ast *n, scope *s)
{
    /* n->type is either AST_INDEX or AST_FIELD */
    if (n->type == AST_FIELD) {
        compile_node(p, n->as.field.m, s);
        ast *key = n->as.field.f;
        emit(p, key->as.name.safe ? OP_GETFIELD_SAFE : OP_GETFIELD, n);
        int i = get_string_idx(p, key->as.name.s);
        emit16(p, i, key);
    } else {
        compile_node(p, n->as.index.m, s);
        compile_node(p, n->as.index.i, s);
        emit(p, OP_GETINDEX, n);
    }
}

static void compile_unary(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.unary.e, s);

    seal_byte opcode;
    switch (n->as.unary.op) {
    case IMOP_UNARY_MINUS:
        opcode = OP_NEG;
        break;
    case IMOP_LOGICAL_NOT:
        opcode = OP_NOT;
        break;
    case IMOP_BITWISE_NOT:
        opcode = OP_BNOT;
        break;
    case IMOP_PREFIX_INC:
    case IMOP_PREFIX_DEC:
    case IMOP_POSTFIX_INC:
    case IMOP_POSTFIX_DEC:
        SEAL_ASSERT(0 && "++ and -- not compiled");
        break;
    }

    emit(p, opcode, n);
}

static void compile_binary(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.bin.l, s);
    compile_node(p, n->as.bin.r, s);

    seal_byte opcode;
    switch (n->as.bin.op) {
    case IMOP_MUL:
        opcode = OP_MUL;
        break;
    case IMOP_DIV:
        opcode = OP_DIV;
        break;
    case IMOP_MOD:
        opcode = OP_MOD;
        break;
    case IMOP_ADD:
        opcode = OP_ADD;
        break;
    case IMOP_SUB:
        opcode = OP_SUB;
        break;
    case IMOP_SHL:
        opcode = OP_SHL;
        break;
    case IMOP_SHR:
        opcode = OP_SHR;
        break;
    case IMOP_LT:
        opcode = OP_LT;
        break;
    case IMOP_LE:
        opcode = OP_LE;
        break;
    case IMOP_GT:
        opcode = OP_GT;
        break;
    case IMOP_GE:
        opcode = OP_GE;
        break;
    case IMOP_EQ:
        opcode = OP_EQ;
        break;
    case IMOP_NE:
        opcode = OP_NE;
        break;
    case IMOP_BITWISE_AND:
        opcode = OP_AND;
        break;
    case IMOP_XOR:
        opcode = OP_XOR;
        break;
    case IMOP_BITWISE_OR:
        opcode = OP_OR;
        break;
    default:
        SEAL_ASSERT(0);
        break;
    }

    emit(p, opcode, n);
}

static void compile_logbin(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.bin.l, s);
    emitn(p, OP_DUP);
    emitn(p, n->as.bin.op == IMOP_AND ? OP_JFALSE : OP_JTRUE);
    int jump = p->code_size;
    emit16dummy(p);
    emitn(p, OP_POP);
    compile_node(p, n->as.bin.r, s);
    jmpreplace16cur(p, jump); 
}

static void compile_ternary(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.ternary.c, s);
    emitn(p, OP_JFALSE);
    int else_jump = p->code_size;
    emit16dummy(p);
    compile_node(p, n->as.ternary.t, s);
    emitn(p, OP_JMP);
    int end_jump = p->code_size;
    emit16dummy(p);
    jmpreplace16cur(p, else_jump);
    compile_node(p, n->as.ternary.e, s);
    jmpreplace16cur(p, end_jump);
}

static seal_byte augop2byte(int op)
{
    seal_byte b = -1;
    switch (op) {
    case IMOP_MUL_ASSIGN:
        b = OP_MUL;
        break;
    case IMOP_DIV_ASSIGN:
        b = OP_DIV;
        break;
    case IMOP_MOD_ASSIGN:
        b = OP_MOD;
        break;
    case IMOP_ADD_ASSIGN:
        b = OP_ADD;
        break;
    case IMOP_SUB_ASSIGN:
        b = OP_SUB;
        break;
    case IMOP_SHL_ASSIGN:
        b = OP_SHL;
        break;
    case IMOP_SHR_ASSIGN:
        b = OP_SHR;
        break;
    case IMOP_AND_ASSIGN:
        b = OP_AND;
        break;
    case IMOP_XOR_ASSIGN:
        b = OP_XOR;
        break;
    case IMOP_OR_ASSIGN:
        b = OP_OR;
        break;
    }
    return b;
}

static void compile_assign_name(proto *p, ast *var, ast *val, int op, scope *s)
{
    if (op == IMOP_ASSIGN) {
        compile_node(p, val, s);
        if (var->as.name.global) {
            int i = get_string_idx(p, var->as.name.s);
            emit(p, OP_SETGLOBAL, var);
            emit16(p, i, var);
        } else {
            h_entry *e = hashmap_search(s->h, var->as.name.s);
            if (e == NULL) {
                /* HASHMAP IS FULL */
                SEAL_ASSERT(0 && "maximum amount of locals is 256");
            }
            if (e->key == NULL) {
                /* VARIABLE IS NOT SET YET */
                int idx = s->h->len;
                hashmap_insert_e(s->h, e, var->as.name.s, SEAL_VINT(idx));
            }
            emit(p, OP_SETLOCAL, var);
            emit(p, e->val.as.integer, var);
        }
    } else {
        int b = augop2byte(op);
        if (var->as.name.global) {
            int str_idx = get_string_idx(p, var->as.name.s);
            emit(p, OP_GETGLOBAL, var);
            emit16(p, str_idx, var);
            compile_node(p, val, s);
            emitn(p, b);
            emit(p, OP_SETGLOBAL, var);
            emit16(p, str_idx, var);
        } else {
            h_entry *e = hashmap_search(s->h, var->as.name.s);
            if (nullhentry(e)) {
                /* TODO: fix error msg */
                SEAL_ASSERT(0 && "cannot assign augmentedly to undeclared variable");
            }
            int local_idx = e->val.as.integer;
            emit(p, OP_GETLOCAL, var);
            emit(p, local_idx, var);
            compile_node(p, val, s);
            emitn(p, b);
            emit(p, OP_SETLOCAL, var);
            emit(p, local_idx, var);
        }
    }
}

static void compile_assign_field(proto *p, ast *var, ast *val, int op, scope *s)
{
    compile_node(p, var->as.field.m, s);
    ast *key = var->as.field.f;
    int i = get_string_idx(p, key->as.name.s);
    if (op == IMOP_ASSIGN) {
        /* compile main part */
        /* compile assigned expr */
        compile_node(p, val, s);
        /* compile OP_SETFIELD */
        emit(p, OP_SETFIELD, key);
        emit16(p, i, key);
    } else {
        emitn(p, OP_DUP);
        emit(p, OP_GETFIELD, key);
        emit16(p, i, key);
        compile_node(p, val, s);
        int b = augop2byte(op);
        emitn(p, b);
        emit(p, OP_SETFIELD, key);
        emit16(p, i, key);
    }
}

static void compile_assign_index(proto *p, ast *var, ast *val, int op, scope *s)
{
    ast *m = var->as.index.m;
    compile_node(p, m, s);
    ast *i = var->as.index.i;
    compile_node(p, i, s);
    if (op == IMOP_ASSIGN) {
        /* compile assigned expr */
        compile_node(p, val, s);
        /* compile OP_SETINDEX */
        emit(p, OP_SETINDEX, m);
    } else {
        emitn(p, OP_COPY);
        emitn(p, 2);
        emitn(p, OP_COPY);
        emitn(p, 2);
        emit(p, OP_GETINDEX, i);
        compile_node(p, val, s);
        int b = augop2byte(op);
        emitn(p, b);
        emit(p, OP_SETINDEX, i);
    }
}

static void compile_assign(proto *p, ast *n, scope *s)
{
    ast *var = n->as.assign.var;
    ast *val = n->as.assign.val;
    int op = n->as.assign.op;

    switch (var->type) {
    case AST_NAME:
        compile_assign_name(p, var, val, op, s);
        break;
    case AST_FIELD:
        compile_assign_field(p, var, val, op, s);
        break;
    case AST_INDEX:
        compile_assign_index(p, var, val, op, s);
        break;
    }
}

static void compile_comma(proto *p, ast *n, scope *s)
{
    ast *l = n->as.bin.l;
    ast *r = n->as.bin.r;

    compile_node(p, l, s);
    emit(p, OP_POP, l);
    compile_node(p, r, s);
}

static void compile_if(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.ifstmt.cond, s);
    emitn(p, OP_JFALSE);
    int next_jump = p->code_size;
    emit16dummy(p);
    compile_node(p, n->as.ifstmt.body, s);
    if (n->as.ifstmt.haselse) {
        emitn(p, OP_JMP);
        int end_jump = p->code_size;
        emit16dummy(p);
        jmpreplace16cur(p, next_jump);
        if (n->as.ifstmt.elsepart->type == AST_IF) {
            compile_if(p, n->as.ifstmt.elsepart, s);
        } else {
            compile_node(p, n->as.ifstmt.elsepart->as.elsestmt.body, s);
        }
        jmpreplace16cur(p, end_jump);
    } else {
        jmpreplace16cur(p, next_jump);
    }
}

static void compile_while(proto *p, ast *n, scope *s)
{
    int begin_pos = p->code_size;
    push_loop(p, begin_pos);

    compile_node(p, n->as.whilestmt.cond, s);
    emitn(p, OP_JFALSE);
    int end_jump = p->code_size;
    emit16dummy(p);
    compile_node(p, n->as.whilestmt.body, s);

    emitn(p, OP_JMP);
    int begin_jump = p->code_size;
    emit16dummy(p);
    jmpreplace16(p, begin_pos, begin_jump);
    jmpreplace16cur(p, end_jump);

    backpatch_stops(p);
    pop_loop(p);
}

static void compile_dowhile(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(0 && "Do not use do-while loops yet");
    int begin_pos = p->code_size;
    compile_node(p, n->as.whilestmt.body, s);

    compile_node(p, n->as.whilestmt.cond, s);
    emitn(p, OP_JTRUE);
    int begin_jump = p->code_size;
    emit16dummy(p);
    jmpreplace16(p, begin_pos, begin_jump);
}

static void compile_for(proto *p, ast *n, scope *s)
{
    (void)p;
    (void)n;
    (void)s;
    SEAL_ASSERT(0);
}

static void compile_skip(proto *p)
{
    emitn(p, OP_JMP);
    int skip_jump = p->code_size;
    emit16dummy(p);
    jmpreplace16(p, cur_loop(p).begin_pos, skip_jump);
}

static void compile_stop(proto *p)
{
    emitn(p, OP_JMP);
    int addr = p->code_size - cur_loop(p).begin_pos;
    SEAL_ASSERT(addr <= SHRT_MAX && addr >= SHRT_MIN);
    emit16(p, cur_loop(p).last_stop, NULL);
    cur_loop(p).last_stop = addr;
}

static void compile_return(proto *p, ast *n, scope *s)
{
    compile_node(p, n->as.retstmt.val, s);
    emit(p, OP_RETURN, n);
}

static void compile_include(proto *p, ast *n)
{
    emit(p, OP_INCLUDE, n);
    int i = get_string_idx(p, n->as.inclstmt.fname);
    emit16(p, i, n);
}

static void compile_node(proto *p, ast *n, scope *s)
{
    if (n->line >= 1) {
        p->last_seen_line = n->line;
    }
    ast *temp;
    switch (tnode(n)) {
    case AST_NOP: break;
    case AST_COMP:
        temp = n->as.comp.stmts;
        while (temp) {
            switch (tnode(temp)) {
            case AST_NULL: case AST_TRUE: case AST_FALSE:
            case AST_INT: case AST_FLOAT: case AST_STRING:
                //break; // do not optimize to let REPL print
            case AST_NAME: case AST_FUNC_DEF: case AST_FUNC_CALL:
            case AST_METH_CALL: case AST_INDEX: case AST_FIELD:
            case AST_LIST: case AST_MAP: case AST_UNARY:
            case AST_BINARY: case AST_LOGBIN:
            case AST_TERNARY: case AST_ASSIGN: case AST_COMMA:
                compile_node(p, temp, s);
                emit(p, OP_POP, temp);
                break;
            default:
                compile_node(p, temp, s);
                break;
            }
            temp = temp->next;
        }
        break;
    case AST_NULL: case AST_TRUE: case AST_FALSE:
    case AST_INT: case AST_FLOAT: case AST_STRING:
        compile_value(p, n);
        break;
    case AST_LIST:
        compile_list(p, n, s);
        break;
    case AST_MAP:
        compile_map(p, n, s);
        break;
    case AST_NAME:
        compile_name(p, n, s);
        break;
    case AST_FUNC_DEF:
        compile_func_def(p, n, s);
        break;
    case AST_FUNC_CALL: case AST_METH_CALL:
        compile_call(p, n, s);
        break;
    case AST_INDEX: case AST_FIELD:
        compile_field(p, n, s);
        break;
    case AST_UNARY:
        compile_unary(p, n, s);
        break;
    case AST_BINARY:
        compile_binary(p, n, s);
        break;
    case AST_LOGBIN:
        compile_logbin(p, n, s);
        break;
    case AST_TERNARY:
        compile_ternary(p, n, s);
        break;
    case AST_ASSIGN:
        compile_assign(p, n, s);
        break;
    case AST_COMMA:
        compile_comma(p, n, s);
        break;
    case AST_IF:
        compile_if(p, n, s);
        break;
    case AST_WHILE:
        compile_while(p, n, s);
        break;
    case AST_DOWHILE:
        compile_dowhile(p, n, s);
        break;
    case AST_FOR:
        compile_for(p, n, s);
        break;
    case AST_SKIP:
        compile_skip(p);
        break;
    case AST_STOP:
        compile_stop(p);
        break;
    case AST_RETURN:
        compile_return(p, n, s);
        break;
    case AST_INCLUDE:
        compile_include(p, n);
        break;
    }
}


/* convert this return type to pointer */
struct chunk compile(struct seal_state *S,
                     struct ast *n,
                     struct seal_hashmap *h,
                     const char *file_name)
{
    struct chunk c = {0};
    proto p = {0};
    p.file_name = file_name;
    scope s = {0};
    s.h = h ? h : hashmap_Nnew(SEAL_LOCAL_MAX);
    compile_node(&p, n, &s);
    if (S &&
        S->repl_mode &&
        p.code_size > 0 &&
        p.code[p.code_size - 1] == OP_POP
    ) {
        p.code[p.code_size - 1] = OP_GETGLOBAL;
        emit16(&p, get_string_idx(&p, "print"), NULL);
        emitn(&p, OP_SWAP);
        emitn(&p, 1);
        emitn(&p, OP_CALL);
        emitn(&p, 1);
        emitn(&p, OP_POP);
    }
    emitn(&p, OP_PUSHNULL);
    emitn(&p, OP_RETURN);

    c.code = p.code;
    c.code_size = p.code_size;
    c.pool = p.pool;
    c.pool_size = p.pool_size;
    c.li = p.li;
    c.li_size = p.li_size;
    c.local_size = s.h->len;

    hashmap_free(s.h, false);

    return c;
}

int get_line(struct chunk *c, seal_byte *ip)
{
    int low = 0;
    int high = c->li_size - 1;
    int line = 1;
    int offset = ip - c->code;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        line_info *li = &c->li[mid];

        if (li->offset <= offset) {
            line = li->line;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return line;
}

#if SEAL_DEBUG

typedef struct {
    const char *const name;
    int operand_size;
} OpSpec;

static const OpSpec op_specs[] = {
    [OP_HALT]  = { "halt",  0 },
    [OP_PUSH]  = { "push",  1 },
    [OP_LPUSH] = { "lpush", 2 },
    [OP_PUSHNULL]  = { "push null", 0 },
    [OP_PUSHTRUE]  = { "push true", 0 },
    [OP_PUSHFALSE] = { "push false", 0 },
    [OP_PUSH8]  = { "push8",  1 },
    [OP_PUSH16] = { "push16", 2 },
    [OP_POP]  = { "pop", 0 },
    [OP_DUP]  = { "dup", 0 },
    [OP_COPY] = { "copy", 0 },
    [OP_SWAP] = { "swap", 1 },
    [OP_JMP]  = { "jump", 2 },
    [OP_JTRUE]   = { "jump if true",  2 },
    [OP_JFALSE]  = { "jump if false", 2 },
    [OP_JNULL]   = { "jump if null", 2 },
    [OP_CALL] = { "call", 1 },
    [OP_RETURN]  = { "return", 0 },
    /* binaries */
    [OP_ADD] = { "add", 0 },
    [OP_SUB] = { "sub", 0 },
    [OP_MUL] = { "mul", 0 },
    [OP_DIV] = { "div", 0 },
    [OP_MOD] = { "mod", 0 },
    [OP_AND] = { "and", 0 },
    [OP_OR]  = { "or",  0 },
    [OP_XOR] = { "xor", 0 },
    [OP_SHL] = { "shift left",  0 },
    [OP_SHR] = { "shift right", 0 },
    [OP_GT]  = { "gt", 0 },
    [OP_GE]  = { "ge", 0 },
    [OP_LT]  = { "lt", 0 },
    [OP_LE]  = { "le", 0 },
    [OP_EQ]  = { "eq", 0 },
    [OP_NE]  = { "ne", 0 },
    /* unaries */
    [OP_NOT]  = { "not", 0 },
    [OP_BNOT] = { "bitwise not", 0 },
    [OP_NEG]  = { "neg", 0 },
    /* symbols */
    [OP_GETGLOBAL] = { "get global", 2 },
    [OP_GETGLOBAL_SAFE] = { "get global safe", 2 },
    [OP_SETGLOBAL] = { "set global", 2 },
    [OP_GETLOCAL]  = { "get local" , 1 },
    [OP_SETLOCAL]  = { "set local" , 1 },
    /* lists and maps */
    [OP_NEWLIST]  = { "new list", 0 },
    [OP_MAKELIST] = { "make list", 1 },
    [OP_PUSHLIST] = { "push list", 1 },
    [OP_NEWMAP]   = { "new map", 0 },
    [OP_MAKEMAP]  = { "make map", 1 },
    [OP_PUSHMAP]  = { "push map", 1 },
    [OP_GETFIELD] = { "get field", 2 },
    [OP_GETFIELD_SAFE] = { "get field safe", 2 },
    [OP_SETFIELD] = { "set field", 2 },
    [OP_GETINDEX] = { "get index", 0 },
    [OP_SETINDEX] = { "set index", 0 },
    /* other */
    [OP_INCLUDE] = { "include", 2 }
};

const char *get_opname(int op)
{
    return op_specs[op].name;
}

void dump_chunk(struct chunk *c)
{
    printf("Total bytes: %d\n", c->code_size);
    int cur_line = 0;
    for (int i = 0; i < c->code_size; i++) {
        int checked_line = get_line(c, &c->code[i]);
        if (cur_line != checked_line) {
            cur_line = checked_line;
            printf("--- line %d ---\n", cur_line);
        }
        seal_byte byte = c->code[i];
        OpSpec op = op_specs[byte];
        printf("[%2d]: %d byte%s | ", i, 1 + op.operand_size, op.operand_size ? "s" : " ");
        printf("%s", op.name);

        if (op.operand_size != 0) {
            putchar(' ');
            switch (byte) {
            case OP_PUSH: case OP_LPUSH: {
                int n = c->code[++i];
                if (byte == OP_LPUSH) {
                    n <<= 8;
                    n |= c->code[++i];
                }
                struct seal_value v = c->pool[n];
                switch (v.type) {
                case SEAL_TINT:
                    printf("%lld", SEAL_AS_INT(v));
                    break;
                case SEAL_TFLOAT:
                    printf("%g", SEAL_AS_FLOAT(v));
                    break;
                case SEAL_TSTRING:
                    printf("\"%s\"", SEAL_AS_STRINGVAL(v));
                    break;
                case SEAL_TFUNCTION:
                    printf("function %s: %p",
                           SEAL_AS_SFUNC(v).name ? SEAL_AS_SFUNC(v).name : "",
                           (void*)SEAL_AS_FUNC(v));
                    dump_chunk(SEAL_AS_SFUNC(v).c);
                    puts("END FUNCTION");
                    break;
                }
                putchar(' ');
                putchar('|');
                putchar(' ');
                printf("%d", n);
                break;
            }
            case OP_GETGLOBAL:
            case OP_GETGLOBAL_SAFE:
            case OP_SETGLOBAL:
            case OP_GETFIELD:
            case OP_GETFIELD_SAFE:
            case OP_SETFIELD:
            case OP_INCLUDE: {
                int n = c->code[++i];
                n <<= 8;
                n |= c->code[++i];
                printf("\"%s\"", c->pool[n].as.string->val);
                putchar(' ');
                putchar('|');
                putchar(' ');
                printf("%d", n);
                break;
            }
            case OP_JMP: case OP_JTRUE: case OP_JFALSE: case OP_JNULL: {
                short n = c->code[++i];
                n <<= 8;
                n |= c->code[++i];
                printf("%d -> [%d]", n, i + 1 + n);
                break;
            }
            default:
                if (op.operand_size == 1) {
                    printf("%d", c->code[++i]);
                } else if (op.operand_size == 2) {
                    int n = c->code[++i];
                    n <<= 8;
                    n |= c->code[++i];
                    printf("%d", n);
                }
                break;
            }
        }
        putchar('\n');
    }
}

void dump_bytecode(struct chunk *c)
{
    printf("Total bytes: %d\n", c->code_size);
    for (int i = 0; i < c->code_size; i++) {
        printf("[%d]: %d\n", i, c->code[i]);
    }
}

#endif
