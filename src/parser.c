#include "parser.h"
#include <stdio.h>


/* Precedence Table
   left-to-right
   ++ -- (postfix increment and decrement)
   ()  (function call)
   []  (indexing)
   .   (field access)
   ->  (method call)
------------------------------------------
   right-to-left
   ++ -- (prefix increment and decrement)
   + -  (unary plus and minus)
   ! not ~  (logical NOT and bitwise NOT)
------------------------------------------
   left-to-right
   * / %
   + -
   << >>
   < <= > >=
   == !=
   &  (bitwise AND)
   ^  (XOR)
   |  (bitwise OR)
   and  (logical AND)
   or   (logical OR)
------------------------------------------
   right-to-left
   if <expr> then <expr> else <expr> (ternary)
   = *= /= %= += -= <<= >>= &= ^= |=  (assignment)
------------------------------------------
   left-to-right
   , (comma)
*/


/* typedefs */
typedef struct token token;
typedef struct ast ast;
typedef struct parser parser;

/* token and parser */
static inline token adv(parser *p)
{
    token t = p->tcur;
    p->tcur = p->tnext;
    p->tnext = lexer_get_token(p->l);
    return t;
}

#define cur(p)   ((p)->tcur)  /* current token */
#define next(p)  ((p)->tnext) /* next token */
#define ttype(t) ((t).type)
#define val(t)   ((t).val)
#define curtype(p)  (ttype(cur(p)))
#define nexttype(p) (ttype(next(p)))

#define match(p, t)     (curtype(p) == (t))
#define matchnext(p, t) (nexttype(p) == (t))
#define matchadv(p, t)  (match(p, t) ? adv(p), 1 : 0)
#define iseof(p) (match(p, TK_EOF))

#define perror(p, t, ...) ( \
    fprintf(stderr, "line %d: ", (t).line), \
    fprintf(stderr, __VA_ARGS__), \
    fputc('\n', stderr), \
    longjmp((p)->l->fail_point, 1) \
)

#define unexpected(p, t) ( \
    perror(p, t, "unexpected \'%s\'", val(t)) \
)

static inline token eat(parser *p, int t)
{
    //printf("%s is eaten\n", tkname(t));
    if (!match(p, t))
        perror(p, cur(p),
               "expected \'%s\' instead of \'%s\'",
               t < FIRST_WORD_TOKEN
                   ? (char[2]) { t, '\0' }
                   : tkname(t),
               curtype(p) < FIRST_WORD_TOKEN
                   ? (char[2]) { curtype(p), '\0' }
                   : tkname(curtype(p)));

    return adv(p);
}

#define eatval(p, t) (val(eat(p, t)))
#define newl(p) (eat(p, TK_NEWLINE))
#define indent(p) (eat(p, TK_INDENT))
#define dedent(p) (eat(p, TK_DEDENT))

/* AST */
static ast __NODE_NOP   = { .type = AST_NOP   };
static ast __NODE_NULL  = { .type = AST_NULL  };
static ast __NODE_TRUE  = { .type = AST_TRUE  };
static ast __NODE_FALSE = { .type = AST_FALSE };

#define NODE_NOP   (&__NODE_NOP)
#define NODE_NULL  (&__NODE_NULL)
#define NODE_TRUE  (&__NODE_TRUE)
#define NODE_FALSE (&__NODE_FALSE)

#define atype(a)  ((a)->type)
#define islval(a) ( \
    atype(a) == AST_NAME  || \
    atype(a) == AST_INDEX || \
    atype(a) == AST_FIELD \
)
#define isinlstmt(a) ( \
    atype(a) == AST_SKIP    || \
    atype(a) == AST_STOP    || \
    atype(a) == AST_RETURN  || \
    atype(a) == AST_INCLUDE \
)
#define apush(arr, size, node) ( \
    (arr)[(size)] = ast_new( \
    (arr)[(size)++] = (node) \
)
#define listpush(a, node) \
    apush((a)->as.l.items, (a)->as.l.size, node)

#define mappush(a, key, node) ( \
    (a)->as.m.keys[(a)->as.m.size] = (key), \
    (a)->as.m.vals[(a)->as.m.size++] = (node) \
)

static ast *ast_new(parser *p, int type)
{
    ast *a = arena_alloc(&p->a, sizeof(ast));
    *a = (ast) {0};
    a->type = type;
    return a;
}

/* parsing */
#define inccondlvl(p) ((p)->cond_lvl++)
#define inclooplvl(p) ((p)->loop_lvl++)
#define incfunclvl(p) ((p)->func_lvl++)

#define deccondlvl(p) ((p)->cond_lvl--)
#define declooplvl(p) ((p)->loop_lvl--)
#define decfunclvl(p) ((p)->func_lvl--)

#define incond(p) ((p)->cond_lvl > 0)
#define inloop(p) ((p)->loop_lvl > 0)
#define infunc(p) ((p)->func_lvl > 0)

#define inblock(p) (incond(p) || inloop(p) || infunc(p))

/* forward declarations */
static ast *parse_expr(parser *p);
static ast *parse_statement(parser *p, int inl);
static ast *parse_inlstmt(parser *p);
#define parse_stmt(p) (parse_statement(p, false))
static ast *parse_stmts(parser *p);
static ast *parse_else(parser *p);

/* definitions */
static ast *parse_return(parser *p)
{
    ast *r = ast_new(p, AST_RETURN);
    eat(p, TK_RETURN);
    r->as.retstmt.val =
        match(p, TK_NEWLINE) ? NODE_NULL : parse_expr(p);
    return r;
}

static ast *parse_include(parser *p)
{
    ast *i = ast_new(p, AST_INCLUDE);
    eat(p, TK_INCLUDE);
    i->as.inclstmt.fname = eatval(p, TK_STRING);
    return i;
}

static ast *parse_if(parser *p, int ternary)
{
    inccondlvl(p);
    ast *i = ast_new(p, AST_IF);
    ast *cond;
    ast *stmt;
    eat(p, TK_IF);
    cond = parse_expr(p);
    if (matchadv(p, TK_THEN)) {
        stmt = parse_inlstmt(p);
        if (match(p, TK_ELSE) && ternary && !isinlstmt(stmt->as.comp.stmts)) {
            adv(p); /* else */
            i->type = AST_TERNARY;
            i->as.ternary.c = cond;
            i->as.ternary.t = stmt->as.comp.stmts;
            i->as.ternary.e = parse_expr(p);
            return i;
        } else {
            i->as.ifstmt.cond = cond;
            i->as.ifstmt.body = stmt;
        }
    } else {
        i->as.ifstmt.cond = cond;
        newl(p);
        indent(p);
            i->as.ifstmt.body = parse_stmts(p);
        dedent(p);
    }
    if (matchnext(p, TK_ELSE)) {
        newl(p);
        i->as.ifstmt.haselse = true;
        i->as.ifstmt.elsepart = parse_else(p);
    } else {
        i->as.ifstmt.haselse = false;
    }
    deccondlvl(p);
    return i;
}

static ast *parse_else(parser *p)
{
    eat(p, TK_ELSE);
    if (match(p, TK_IF))
        return parse_if(p, false);
    
    inccondlvl(p);
    ast *e = ast_new(p, AST_ELSE);
    if (matchadv(p, TK_NEWLINE)) {
        indent(p);
            e->as.elsestmt.body = parse_stmts(p);
        dedent(p);
    }
    deccondlvl(p);
    return e;
}

static ast *parse_while(parser *p)
{
    inclooplvl(p);
    ast *w = ast_new(p, AST_WHILE);
    eat(p, TK_WHILE);
    w->as.whilestmt.cond = parse_expr(p);
    if (matchadv(p, TK_DO)) {
        w->as.whilestmt.body = parse_inlstmt(p);
    } else {
        newl(p);
        indent(p);
            w->as.whilestmt.body = parse_stmts(p);
        dedent(p);
    }
    declooplvl(p);
    return w;
}

static ast *parse_dowhile(parser *p)
{
    inclooplvl(p);
    ast *w = ast_new(p, AST_DOWHILE);
    eat(p, TK_DO);
    if (!matchadv(p, TK_NEWLINE)) {
        w->as.whilestmt.body = parse_inlstmt(p);
    } else {
        newl(p);
        indent(p);
            w->as.whilestmt.body = parse_stmts(p);
        dedent(p);
    }
    eat(p, TK_WHILE);
    w->as.whilestmt.cond = parse_expr(p);
    declooplvl(p);
    return w;
}

static ast *parse_for(parser *p)
{
    inclooplvl(p);
    declooplvl(p);
    /* TODO */
    return NODE_NOP;
}

static ast *parse_inlstmt(parser *p)
{
    ast *c = ast_new(p, AST_COMP);
    c->as.comp.size = 2;
    c->as.comp.stmts = parse_statement(p, true);
    c->as.comp.stmts->next = NODE_NOP;
    return c;
}

static ast *parse_statement(parser *p, int inl)
{
    switch (curtype(p)) {
    case TK_IF:
        if (inl) goto error;
        return parse_if(p, true);
    case TK_WHILE:
        if (inl) goto error;
        return parse_while(p);
    case TK_DO:
        if (inl) goto error;
        return parse_dowhile(p);
    case TK_FOR:
        if (inl) goto error;
        return parse_for(p);
    case TK_SKIP:
        if (!inloop(p)) goto error;
        adv(p);
        return ast_new(p, AST_SKIP);
    case TK_STOP:
        if (!inloop(p)) goto error;
        adv(p);
        return ast_new(p, AST_STOP);
    case TK_RETURN:
        if (!infunc(p)) goto error;
        return parse_return(p);
    case TK_INCLUDE:
        return parse_include(p);
    case TK_DEDENT:
        return NODE_NOP;
    case TK_EOF:
        return NODE_NOP;
    default:
        return parse_expr(p);
    }
error:
    unexpected(p, cur(p));
    return NULL;
}

static ast *parse_stmts(parser *p)
{
    ast *c = ast_new(p, AST_COMP);
    ast *head = NULL;
    ast *tail = NULL;
    c->as.comp.size = 0;
    //while (!iseof(p)) {
    for (;;) { /* to get NOP at the end of main block */
        c->as.comp.size++;
        ast *s = parse_stmt(p);
        /* OPTIMIZE
         * later on, check if it is indent or eof to save space
         */
        if (!head) {
            head = tail = s;
        } else {
            tail->next = s;
            tail = s;
        }
        //printf("%s\n", val(next(p)));
        if (atype(s) == AST_NOP)
            break;

        newl(p);
    }
    c->as.comp.stmts = head;
    if ((!inblock(p) && !iseof(p)))
        unexpected(p, cur(p));

    return c;
}

static ast *parse_list(parser *p)
{
    ast *l = ast_new(p, AST_LIST);
    ast *head = NULL;
    ast *tail = NULL;
    l->as.l.size  = 0;
    eat(p, '[');
    while (!match(p, ']')) {
        l->as.l.size++;
        ast *i = parse_expr(p);
        if (!head) {
            head = tail = i;
        } else {
            tail->next = i;
            tail = i;
        }
        if (!matchadv(p, ','))
            break;
    }
    l->as.l.items = head;
    eat(p, ']');
    return l;
}

static ast *parse_map(parser *p)
{
    ast *m = ast_new(p, AST_MAP);
    ast *head = NULL;
    ast *tail = NULL;
    m->as.m.size = 0;
    eat(p, '{');
    while (!match(p, '}')) {
        m->as.m.size++;
        const char *key = eatval(p, TK_ID);
        eat(p, '=');
        ast *val = parse_expr(p);
        ast *pair = ast_new(p, -1);
        pair->as.pair.key = key;
        pair->as.pair.val = val;
        if (!head) {
            head = tail = pair;
        } else {
            tail->next = pair;
            tail = pair;
        }
        if (!matchadv(p, ','))
            break;
    }
    eat(p, '}');
    m->as.m.pairs = head;
    return m;
}

static ast *parse_name(parser *p, int global)
{
    ast *n = ast_new(p, AST_NAME);
    n->as.name.global = global;
    n->as.name.s = eatval(p, TK_ID);
    n->as.name.safe = matchadv(p, '?');
    return n;
}

static ast *parse_int(parser *p)
{
    ast *i = ast_new(p, AST_INT);
    const char *lit = val(adv(p));
    seal_int val = 0;

    if (*lit == '0' && lit[1] != '\0' && !(lit[1] >= '0' && lit[1] <= '9')) {
        /* not base 10 */
        int base;
        lit++; /* skip */
        switch (*lit) {
        case 'b': case 'B':
            base = 2;
            break;
        case 'o': case 'O':
            base = 8;
            break;
        case 'x': case 'X':
            base = 16;
            break;
        }
        lit++; /* skip */
        while (*lit) {
            int d = (*lit - '0' > 9
                     ? (*lit >= 'a'
                        ? *lit - 'a' + 10
                        : *lit - 'A' + 10)
                     : *lit - '0'); /* binary or octal */

            val = val * base + d;
            lit++;
        }
    } else {
        while (*lit) {
            val = val * 10 + *lit++ - '0';
        }
    }

    i->as.i = val;
    return i;
}

static ast *parse_float(parser *p)
{
    ast *f = ast_new(p, AST_FLOAT);
    const char *lit = val(adv(p));

    f->as.f = strtod(lit, NULL);

    return f;
}

static ast *parse_string(parser *p)
{
    ast *s = ast_new(p, AST_STRING);
    s->as.s = val(adv(p));
    return s;
}

static ast *parse_func_def(parser *p)
{
    incfunclvl(p);
    ast *f = ast_new(p, AST_FUNC_DEF);
    eat(p, TK_DEFINE);

    f->as.func.name = match(p, TK_ID) ? val(adv(p)) : NULL;

    ast *head = NULL;
    ast *tail = NULL;
    f->as.func.psize = 0;
    eat(p, '(');
    while (!match(p, ')')) {
        f->as.func.psize++;
        ast *n = ast_new(p, AST_NAME);
        n->as.name.s = eatval(p, TK_ID); 
        if (!head) {
            head = tail = n;
        } else {
            tail->next = n;
            tail = n;
        }
        if (!match(p, ')'))
            eat(p, ',');
    }
    f->as.func.params = head;
    eat(p, ')');

    /* lambda */
    if (!match(p, TK_NEWLINE)) {
        ast *r = ast_new(p, AST_RETURN);
        r->as.retstmt.val = parse_expr(p);
        f->as.func.body = r;

        decfunclvl(p);
        return f;
    }

    newl(p);
    indent(p);
        f->as.func.body = parse_stmts(p);
    dedent(p);

    decfunclvl(p);
    return f;
}

static ast *parse_primary(parser *p)
{
    ast *main = NULL;
    switch (curtype(p)) {
    case '(':
        adv(p);
        main = parse_expr(p);
        eat(p, ')');
        break;
    case '[':
        main = parse_list(p);
        break;
    case '{':
        main = parse_map(p);
        break;
    case '$':
        adv(p);
        main = parse_name(p, true);
        break;
    case TK_ID:
        main = parse_name(p, false);
        break;
    case TK_NULL:
        adv(p);
        main = NODE_NULL;
        break;
    case TK_TRUE:
        adv(p);
        main = NODE_TRUE;
        break;
    case TK_FALSE:
        adv(p);
        main = NODE_FALSE;
        break;
    case TK_INT:
        main = parse_int(p);
        break;
    case TK_FLOAT:
        main = parse_float(p);
        break;
    case TK_STRING:
        main = parse_string(p);
        break;
    case TK_DEFINE:
        main = parse_func_def(p);
        break;
    default:
        perror(p, cur(p), "invalid expression: \'%s\'",
           curtype(p) < FIRST_WORD_TOKEN
               ? (char[2]) { curtype(p), '\0' }
               : val(cur(p)));
    }
    
    return main;
}

static ast *parse_call(parser *p, int meth)
{
    ast *c = ast_new(p, meth ? AST_METH_CALL : AST_FUNC_CALL);
    if (meth && !match(p, '('))
        return c;

    ast *head = NULL;
    ast *tail = NULL;
    c->as.call.argc = 0;
    eat(p, '(');
    while (!match(p, ')')) {
        c->as.call.argc++;
        ast *a = parse_expr(p);
        if (!head) {
            head = tail = a;
        } else {
            tail->next = a;
            tail = a;
        }
        if (!match(p, ')'))
            eat(p, ',');
    }
    c->as.call.argv = head;
    eat(p, ')');

    return c;
}

static ast *parse_postfix(parser *p)
{
    ast *main = parse_primary(p);

    for (;;) {
        ast *node;
        switch (curtype(p)) {
        case '(':
            node = parse_call(p, false);
            node->as.call.f = main;
            main = node;
            break;
        case '[':
            adv(p);
            node = ast_new(p, AST_INDEX);
            node->as.index.m = main;
            node->as.index.i = parse_expr(p);
            main = node;
            eat(p, ']');
            break;
        case '.':
            adv(p);
            node = ast_new(p, AST_FIELD);
            node->as.field.m = main;
            node->as.field.f = parse_name(p, false);
            main = node;
            break;
        case TK_ARROW: /* -> */
            adv(p);
            node = ast_new(p, AST_FIELD);
            node->as.field.m = main;
            node->as.field.f = parse_name(p, false);
            main = node;

            node = parse_call(p, true);
            node->as.call.f = main;
            main = node;
            break;
        case TK_INC: TK_DEC:
            if (!islval(main))
                perror(p, cur(p),
                       "\'%s\' operator requires assignable value",
                       val(cur(p)));

            node = ast_new(p, AST_UNARY);
            node->as.unary.op = ttype(adv(p)) == TK_INC
                                    ? IMOP_POSTFIX_INC
                                    : IMOP_POSTFIX_DEC
                                ;
            node->as.unary.e = main;
            main = node;
            break;
        default:
            return main;
        }
    }
}

static ast *parse_unary(parser *p)
{
    ast *u;
    int op = -1;
    int reqlval = false;
    token t = cur(p);
    switch (ttype(t)) {
    case '+':
        adv(p);
        return parse_unary(p);
    case '-':
        op = IMOP_UNARY_MINUS;
        break;
    case '!': case TK_NOT:
        op = IMOP_LOGICAL_NOT;
        break;
    case '~':
        op = IMOP_BITWISE_NOT;
        break;
    case TK_INC:
        op = IMOP_PREFIX_INC;
        reqlval = true;
        break;
    case TK_DEC:
        op = IMOP_PREFIX_DEC;
        reqlval = true;
        break;
    }
    if (op == -1)
        return parse_postfix(p);

    adv(p);
    u = ast_new(p, AST_UNARY);
    u->as.unary.op = op;
    u->as.unary.e  = reqlval ? parse_postfix(p) : parse_unary(p);

    if (reqlval && !islval(u->as.unary.e))
        perror(p, t,
               "\'%s\' operator requires assignable value",
               val(t));

    return u;
}

struct prec {
    int prec;
    int tk;
    int op;
};

static const struct prec prec_table[] = {
    /* 0 */
    { 0, '*', IMOP_MUL },
    { 0, '/', IMOP_DIV },
    { 0, '%', IMOP_MOD },
    /* 1 */
    { 1, '+', IMOP_ADD },
    { 1, '-', IMOP_SUB },
    /* 2 */
    { 2, TK_SHL, IMOP_SHL },
    { 2, TK_SHR, IMOP_SHR },
    /* 3 */
    { 3, '<', IMOP_LT },
    { 3, TK_LE, IMOP_LE },
    { 3, '>', IMOP_GT },
    { 3, TK_GE, IMOP_GE },
    /* 4 */
    { 4, TK_EQ, IMOP_EQ },
    { 4, TK_NE, IMOP_NE },
    /* 5 */
    { 5, '&', IMOP_BITWISE_AND },
    /* 6 */
    { 6, '^', IMOP_XOR },
    /* 7 */
    { 7, '|', IMOP_BITWISE_OR },
    /* 8 */
    { 8, TK_AND, IMOP_AND },
    /* 9 */
    { 9, TK_OR, IMOP_OR }
};

static const size_t prec_size = sizeof(prec_table) / sizeof(prec_table[0]);

static inline const struct prec *tkprec(int tk)
{
    for (int i = 0; i < prec_size; i++)
        if (prec_table[i].tk == tk)
            return &prec_table[i];

    return NULL;
}

#define MIN_PREC 0
#define MAX_PREC 9

static ast *parse_bin(parser *p, int max_prec)
{
    ast *l = parse_unary(p);

    for (;;) {
        const struct prec *pr = tkprec(curtype(p)); 
        if (!pr || max_prec < pr->prec)
            break;

        adv(p);
        ast *b = ast_new(p, AST_BINARY);
        b->as.bin.op = pr->op;
        b->as.bin.l = l;
        b->as.bin.r = parse_bin(p, pr->prec - 1);
        l = b;
    }

    return l;
}

static ast *parse_ternary(parser *p)
{
    ast *t = ast_new(p, AST_TERNARY);

    eat(p, TK_IF);
    t->as.ternary.c = parse_expr(p);
    eat(p, TK_THEN);
    t->as.ternary.t = parse_expr(p);
    eat(p, TK_ELSE);
    t->as.ternary.e = parse_expr(p);

    return t;
}

static ast *parse_assign(parser *p)
{
    ast *l = parse_bin(p, MAX_PREC);
    int op;
    if (match(p, '=')) {
        op = IMOP_ASSIGN;
    } else if (curtype(p) > TK_MUL_ASSIGN && curtype(p) < TK_OR_ASSIGN) {
        op = IMOP_ASSIGN + (curtype(p) - TK_MUL_ASSIGN + 1);
    } else {
        return l;
    }
    if (!islval(l)) {
        return l;
    }

    adv(p);
    ast *a = ast_new(p, AST_ASSIGN);
    a->as.assign.op = op;
    a->as.assign.var = l;
    a->as.assign.val = parse_expr(p);

    return a;
}

static ast *parse_expr(parser *p)
{
    if (match(p, TK_IF))
        return parse_ternary(p);

    return parse_assign(p);
}


void parser_init(struct parser *p, struct lexer *l)
{
    p->l = l;
    p->tcur  = lexer_get_token(l);
    p->tnext = lexer_get_token(l);
    p->a = arena_new(NULL);
    p->node  = NULL;
    p->cond_lvl = p->loop_lvl = p->func_lvl = 0;
}

struct ast *parse(parser *p)
{
    return parse_stmts(p);
}
