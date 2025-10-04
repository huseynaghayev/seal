#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"
#include "arena.h"
#include "ast.h"


struct parser {
    struct lexer *l;
    struct token tcur, tnext;
    struct arena *a;
    struct ast *node;
    int cond_lvl;
    int loop_lvl;
    int func_lvl;
};


void parser_init(struct parser *p, struct lexer *l);
struct ast *parse(struct parser *p);


#endif /* PARSER_H */
