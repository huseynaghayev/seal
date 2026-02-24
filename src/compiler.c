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
} proto;

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
    short __i = (a) - ((b) + 2); \
    (p)->code[b] = __i >> 8; \
    (p)->code[(b) + 1] = __i;  \
} while (0)

#define jmpreplace16cur(p, pos) jmpreplace16(p, (p)->code_size, pos)

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

static struct seal_string *alloc_seal_string(const char *s)
{
    struct seal_string *v = SEAL_MALLOC(sizeof(struct seal_string));
    *v = (struct seal_string) {
        .val = s,
        .len = strlen(s),
        .collect = false,
    };

    return v;
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
    SEAL_ASSERT(0);
}

static void compile_map(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(0);
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
    SEAL_ASSERT(0);
}

static void compile_call(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(n->type == AST_FUNC_CALL);
    SEAL_ASSERT(n->as.call.argc <= 255);
    compile_node(p, n->as.call.f, s); /* function */

    ast *arg = n->as.call.argv;
    while (arg) { /* compile arguments */
        compile_node(p, arg, s);
        arg = arg->next;
    }

    emit(p, OP_CALL, n); /* call instruction */
    emit(p, n->as.call.argc, n); /* number of arguments */
}

static void compile_field(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(0);
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
    SEAL_ASSERT(0);
}

static void compile_assign(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(n->as.assign.op == IMOP_ASSIGN);
    SEAL_ASSERT(n->as.assign.var->type == AST_NAME);

    compile_node(p, n->as.assign.val, s);

    ast *var = n->as.assign.var;
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
}

static void compile_dowhile(proto *p, ast *n, scope *s)
{
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
    SEAL_ASSERT(0);
}

static void compile_skip(proto *p)
{
    SEAL_ASSERT(0);
}

static void compile_stop(proto *p)
{
    SEAL_ASSERT(0);
}

static void compile_return(proto *p, ast *n, scope *s)
{
    SEAL_ASSERT(0);
}

static void compile_include(proto *p, ast *n)
{
    SEAL_ASSERT(0);
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
                break;
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
struct chunk compile(struct ast *n)
{
    struct chunk c = {0};
    proto p = {0};
    scope s = {0};
    s.h = hashmap_Nnew(SEAL_LOCAL_MAX);
    compile_node(&p, n, &s);
    emitn(&p, OP_HALT);

    c.code = p.code;
    c.code_size = p.code_size;
    c.pool = p.pool;
    c.pool_size = p.pool_size;
    c.li = p.li;
    c.li_size = p.li_size;
    c.local_size = s.h->len;

    hashmap_free(s.h, false, false);

    return c;
}

int get_line(struct chunk *c, int ip)
{
    int low = 0;
    int high = c->li_size - 1;
    int line = 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        line_info *li = &c->li[mid];

        if (li->offset <= ip) {
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
};

void dump_chunk(struct chunk *c)
{
    printf("Total bytes: %d\n", c->code_size);
    int cur_line = 0;
    for (int i = 0; i < c->code_size; i++) {
        int checked_line = get_line(c, i);
        if (cur_line != checked_line) {
            cur_line = checked_line;
            printf("--- line %d ---\n", cur_line);
        }
        seal_byte byte = c->code[i];
        OpSpec op = op_specs[byte];
        printf("[%2d]: %d bytes | ", i, 1 + op.operand_size);
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
                printf("\"%s\"", c->pool[n].as.string->val);
                putchar(' ');
                putchar('|');
                putchar(' ');
                printf("%d", n);
                break;
            }
            case OP_GETGLOBAL: case OP_GETGLOBAL_SAFE: case OP_SETGLOBAL: {
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
                printf("%d", n);
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
