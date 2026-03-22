#ifndef LEXER_H
#define LEXER_H


#include <limits.h>
#include "value.h"

/*
 * single char tokens' type is their ascii value
 */
#define FIRST_WORD_TOKEN  (CHAR_MAX + 1)
#define NUM_WORD_TOKENS   (TK_INT - TK_IF + 1)
#define NUM_NONSYM_TOKENS (TK_ID - TK_IF + 1)

/*
 * one character tokens:
 *     $ ! , . + - * / % ~ ^ | & ? : < > = ( ) [ ] { }
 */
enum {
    TK_IF = FIRST_WORD_TOKEN, TK_THEN, TK_ELSE, /* if, then, else */
    TK_WHILE, TK_DO, TK_FOR, TK_IN, /* while, do, for, in */
    TK_SKIP, TK_STOP, /* skip, stop */
    TK_DEFINE, TK_RETURN, TK_INCLUDE, /* define, return, include */
    TK_NULL, TK_TRUE, TK_FALSE, /* null, true, false */
    TK_AND, TK_OR, TK_NOT, /* and, or, not */
    /* literals */
    TK_INT, TK_FLOAT, TK_STRING, TK_ID,
    /* control */
    TK_INDENT, TK_DEDENT, TK_NEWLINE,
    TK_EOF, /* end of file */
    /* non-word tokens */
    TK_EQ, TK_NE, TK_GE, TK_LE, /* ==, !=, >=, <= */
    TK_ARROW, TK_DPERIOD, TK_ELLIP, /* ->, .., ... */
    TK_SHL, TK_SHR, /* <<, >> */
    TK_INC, TK_DEC, /* prefix/postfix operators (++, --) */
    /* compound assignment operators */
    TK_MUL_ASSIGN, TK_DIV_ASSIGN, TK_MOD_ASSIGN, /* *=, /=, %= */
    TK_ADD_ASSIGN, TK_SUB_ASSIGN, /* +=, -= */
    TK_SHL_ASSIGN, TK_SHR_ASSIGN, /* <<=, >>= */
    TK_AND_ASSIGN, TK_XOR_ASSIGN, TK_OR_ASSIGN, /* &=, ^=, |= */
};


struct token {
    int type;
    const char *val;
    int line;
};


#define MAX_PAREN_LEVEL    64
#define MAX_CACHED_TK_SIZE 32 // fix

struct lexer {
    struct seal_state *S; /* for jumping */
    const char *src; /* code */
    struct seal_hashmap *lexemes; /* to avoid duplicate strings */
    int i; /* current */
    int line, col; /* info about line and column */
    struct token tokcur; /* current token */
    int cur_indent; /* to track indentation level */
    char paren_stk[MAX_PAREN_LEVEL];
    int parenline_stk[MAX_PAREN_LEVEL];
    int indent_stk[SEAL_MAX_INDENT_LEVEL];
    int paren_count;
    int indent_count;
    struct token cachedtks[MAX_CACHED_TK_SIZE];
    int cachedtk_len;
    bool seen_word;
    bool tkaftercom; /* if any token comes after comment block */
};

struct token lexer_get_token(struct lexer *l);
void lexer_reset(struct lexer *l);
void lexer_init(struct lexer *l, const char *src);
const char *tkname(int type);


#endif /* LEXER_H */
