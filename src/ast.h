#ifndef AST_H
#define AST_H


#include "sealconf.h"


enum {
    AST_NOP,  /* indicates end of node (no operation) */
    /* values */
    AST_NULL, AST_TRUE, AST_FALSE, /* null, true, false */
    AST_INT, AST_FLOAT, /* numerics */
    AST_STRING, AST_LIST, AST_MAP, /* references */
    AST_NAME, /* name */
    AST_FUNC_DEF, /* function definition */
    AST_FUNC_CALL, /* function call (a()) */
    AST_METH_CALL, /* method call (a->b) */
    AST_INDEX, /* a[0] */
    AST_FIELD, /* a.b */
    /* compound statements */
    AST_COMP, /* compound (program) */
    AST_IF, AST_ELSE, /* conditions */
    AST_WHILE, AST_DOWHILE, AST_FOR, /* loops */
    /* simple statements */
    AST_SKIP, AST_STOP, AST_RETURN, AST_INCLUDE,

    /* operations (precedence goes higher to lower) */

    AST_UNARY,   /* left-to-right
                    ++ -- (postfix increment and decrement)
                  -------------------
                    right-to-left
                    ++ -- (prefix increment and decrement)
                    + -  (unary plus and minus)
                    ! not ~  (logical NOT and bitwise NOT)
                 */

    AST_BINARY,  /* left-to-right
                    * / %
                    + -
                    << >>
                    < <=
                    > >=
                    == !=
                    &  (bitwise AND)
                    ^  (XOR)
                    |  (bitwise OR)
                 */

    AST_LOGBIN,  /* left-to-right
                    and  (logical AND)
                    or   (logical OR)
                 */

    AST_TERNARY, /* right-to-left
                    if <expr> then <expr> else <expr>
                 */

    AST_ASSIGN,  /* right-to-left
                    = *= /= %= += -= <<= >>= &= ^= |=
                 */
    AST_COMMA
};

enum { /* immediate operations (for parsing) */
    /* unaries */
    IMOP_POSTFIX_INC, /* a++ */
    IMOP_POSTFIX_DEC, /* a-- */
    IMOP_PREFIX_INC,  /* ++b */
    IMOP_PREFIX_DEC,  /* --a */
    /* unary plus is not defined,
     * because writing +a does not make any sense
     */
    IMOP_UNARY_MINUS, /* -a */
    IMOP_LOGICAL_NOT, /* !a (or) not a (even combined: !not!!not a) */
    IMOP_BITWISE_NOT, /* ~a */

    /* binaries */
    IMOP_MUL, IMOP_DIV, IMOP_MOD, /* a * b / c % d */
    IMOP_ADD, IMOP_SUB, /* a + b - c */
    IMOP_SHL, IMOP_SHR, /* a << b >> c */
    IMOP_LT, IMOP_LE,   /* a < b <= c */
    IMOP_GT, IMOP_GE,   /* a > b >= c */
    IMOP_EQ, IMOP_NE,   /* a == b != c */
    IMOP_BITWISE_AND,   /* a & b */
    IMOP_XOR,           /* a ^ b */
    IMOP_BITWISE_OR,    /* a | b */

    /* logical binaries */
    IMOP_AND, /* a and b */
    IMOP_OR,  /* a or b */

    /* assignments */
    IMOP_ASSIGN, /* = */
    IMOP_MUL_ASSIGN, IMOP_DIV_ASSIGN, IMOP_MOD_ASSIGN,
    IMOP_ADD_ASSIGN, IMOP_SUB_ASSIGN,
    IMOP_SHL_ASSIGN, IMOP_SHR_ASSIGN,
    IMOP_AND_ASSIGN, IMOP_XOR_ASSIGN, IMOP_OR_ASSIGN,

    /* comma */
    IMOP_COMMA
};

struct ast {
    int type;
    int line;
    struct ast *next;
    union {
        /* values */
        seal_int    i; /* integer */
        seal_float  f; /* floating point */
        const char *s; /* string */

        struct { /* list (array) */
            struct ast *items; /* linked */
            size_t size;
        } l;

        struct { /* pair of map (linked) */
            const char *key;
            struct ast *val;
        } pair;

        struct { /* map (hashmap) */
            struct ast *pairs; /* linked to 'k' struct */
            size_t size;
        } m;

        struct {
            const char *s;
            int global;
            int safe;
        } name;

        struct { /* function definition */
            const char *name; /* if NULL ptr, then anonymous */
            int global;
            struct ast *params; /* parameter name (stored in string, linked) */
            int psize;
            struct ast *body;
        } func;

        struct { /* used for both function and method calls */
            struct ast *f;
            struct ast *argv; /* linked */
            size_t argc;
        } call;

       struct { /* subscript indexing */
            struct ast *m;
            struct ast *i;
        } index;

       struct { /* field indexing */
            struct ast *m;
            struct ast *f;
        } field;

       /* compound statements */
       struct { /* compound statement */
           struct ast *stmts; /* linked */
           size_t size;
       } comp;

       struct { /* if */
           struct ast *cond;
           struct ast *body;
           struct ast *elsepart;
           int haselse;
       } ifstmt;

       struct { /* else */
           struct ast *body;
       } elsestmt;

       struct { /* used for both while and do-while statements */
           struct ast *cond;
           struct ast *body;
       } whilestmt;

       struct { /* for */
           const char *it; /* TODO: key, value pairs */
           struct ast *ited;
           struct ast *body;
       } forstmt;

       /* simple statements */
       struct {
           struct ast *val; /* if NULL ptr, then return null */
       } retstmt;

       struct {
           const char *fname; /* file name */
       } inclstmt;

       /* operations */
       struct {
           int op;
           struct ast *e;
       } unary;

       struct { /* used for binary, logical binary and comma expressions */
           int op;
           struct ast *l, *r;
       } bin;

       struct {
           struct ast *c, *t, *e;
       } ternary;

       struct {
           int op;
           struct ast *var, *val;
       } assign;
    } as;
};

#if SEAL_DEBUG
void dump_ast(struct ast *node, int i);
#endif

#endif /* AST_H */
