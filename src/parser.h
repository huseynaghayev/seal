#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"
#include "arena.h"
#include "ast.h"


struct parser {
    struct lexer *l;
    struct token tcur, tnext;
    struct arena *a;
    int cond_lvl;
    struct {
        bool is_loop_stmt_used; /* did use skip or stop */
    } loop[SEAL_MAX_LOOP_DEPTH];
    int loop_lvl;
    int func_lvl;
};


void parser_init(struct parser *p, struct lexer *l);
struct ast *parse(struct parser *p);


#endif /* PARSER_H */
