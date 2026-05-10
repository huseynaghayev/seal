#include "lexer.h"
#include <stdio.h>
#include "state.h"


#define LEXEME_START_CAP 8

/* for optimizing */
#define SHORTEST_TK_LEN 2 /* if, do, in, or */
#define LONGEST_TK_LEN  7 /* include */

/* for lexing numbers */
#define NUM_DEC 0
#define NUM_BIN 1
#define NUM_OCT 2
#define NUM_HEX 3
#define NUM_FLT 4

/* order is reserved */
static const char *const token_names[] = {
    "if", "then", "else",
    "while", "do", "for", "in",
    "skip", "stop",
    "define", "return", "include",
    "null", "true", "false",
    "and", "or", "not",
    "<integer>", "<float>", "<string>", "<identifier>",
    "<indentation>", "<dedentation>", "<newline>",
    "<end of file>",
    "==", "!=", ">=", "<=",
    "->", "..", "...",
    "<<", ">>",
    "++", "--",
    "*=", "/=", "%=",
    "+=", "-=",
    "<<=", ">>=",
    "&=", "^=", "|=",
};

/* typedef */
typedef struct token token;
typedef struct lexer lexer;

#define lerrorln(l, ln, ...) ( \
    seal_set_errcode((l)->S, SEAL_ERR_LEX), \
    seal_error((l)->S, ln, __VA_ARGS__) \
)
#define lerror(l, ...) lerrorln(l, (l)->line, __VA_ARGS__)

#define isnum(c) ((c) >= '0' && (c) <= '9')
#define isbin(c) ((c) == '0' || (c) == '1')
#define isoct(c) ((c) >= '0' && (c) <= '7')
#define ishex(c) ( \
    ((c) >= 'a' && (c) <= 'f') || \
    ((c) >= 'A' && (c) <= 'F') || \
    isnum(c) \
)

#define isalpha(c) ( \
    ((c) >= 'a' && (c) <= 'z') || \
    ((c) >= 'A' && (c) <= 'Z') \
)

#define isalnum(c) (isalpha(c) || isnum(c))
#define isidchr(c) (isalnum(c) || c == '_')

#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

/* token */
#define toklit(type, val) (token) { type, val, l->line } /* 'l'
                                                          * is lexer
                                                          */
#define tokword(type)     toklit(type, token_names[type - FIRST_WORD_TOKEN])
#define tokchar(type)     toklit(type, NULL)

#define TNEWLINE tokword(TK_NEWLINE)
#define TINDENT  tokword(TK_INDENT)
#define TDEDENT  tokword(TK_DEDENT)
#define TEOF     tokword(TK_EOF)

/* lexer */
#define cur(l)      ((l)->src[(l)->i])
#define peek(l, cc) ((l)->src[(l)->i + (cc)])
#define iseof(l)    (cur(l) == '\0')

#define inc_col(l)   ((l)->col++)
#define newline(l)   ((l)->col = 1, (l)->line++)

#define advance(l) ( \
    !iseof(l) \
    ? inc_col(l), (l)->src[(l)->i++] \
    : '\0' \
)

#define match(l, c)    (cur(l) == (c))
#define matchadv(l, c) (match(l, c) ? advance(l) : '\0')

#define chr1or2tk(l, c, e, type) (matchadv(l, e) ? tokword(type) : tokchar(c))

/* indentation level -> ilvl */
#define resetilvl(l) ((l)->cur_indent = 0)
#define previlvl(l)  ((l)->indent_stk[(l)->indent_count - 1])
#define curilvl(l)   ((l)->cur_indent)
#define hasilvl(l)   ((l)->indent_count > 0)

#define pushilvl(l) do { \
    if ((l)->indent_count >= SEAL_MAX_INDENT_LEVEL) \
        lerror(l, "maximum indentation level has reached"); \
    (l)->indent_stk[(l)->indent_count++] = curilvl(l); \
} while (0)

#define popilvl(l) (/*(l)->indent_stk[*/--(l)->indent_count/*]*/)

/* parenthesis */
#define isinparen(l)     ((l)->paren_count > 0)
#define isfirstparen(l)  ((l)->paren_count == 1)
#define isopenparentk(t) ( \
    (t).type == '(' || (t).type == '[' || (t).type == '{' \
)
#define isclosingparentk(t) ( \
    (t).type == ')' || (t).type == ']' || (t).type == '}' \
)

#define parenpush(l, p) do { \
    if ((l)->paren_count >= MAX_PAREN_LEVEL) \
        lerror(l, "maximum parenthesis depth has reached"); \
    (l)->parenline_stk[(l)->paren_count] = (l)->line; \
    (l)->paren_stk[(l)->paren_count] = (p); \
    (l)->paren_count++; \
} while (0)

#define parenpop(l, p) do { \
    if (--(l)->paren_count < 0) \
        lerror(l, "unmatched \'%c\'", (p)); \
    char popped = (l)->paren_stk[l->paren_count]; \
    if ((p) - popped > 2) \
        lerror(l, "\'%c\' does not balance \'%c\'", (p), popped); \
} while (0)

/* comment ignoring */
#define skiplcom(l) do { \
    while (!iseof(l)) { \
        if (!match(l, '\n')) \
            advance(l); \
        else \
            break; \
    } \
} while (0)

#define skipccom(l) do { \
    int line = (l)->line; \
    while (!iseof(l)) { \
        if (matchadv(l, '\n')) { \
            newline(l); continue; \
        } \
        if (cur(l) == '*' && peek(l, 1) == '/') \
            break; \
        advance(l); \
    } \
    if (!matchadv(l, '*') || !matchadv(l, '/')) \
        lerrorln(l, line, "unterminated comment block"); \
    if (!(l)->seen_word && !isinparen(l)) \
        (l)->tkaftercom = true; \
} while (0)

/* caching tokens */
#define cachetk(l, t) do { \
    if ((l)->cachedtk_len >= MAX_CACHED_TK_SIZE) \
        lerror(l, "cannot cache anymore token"); \
    (l)->cachedtks[(l)->cachedtk_len++] = (t); \
} while (0)

#define hascache(l) ((l)->cachedtk_len > 0)

#define popcachedtk(l, p) do { \
    *(p) = (l)->cachedtks[0]; \
    for (int i = 1; i < (l)->cachedtk_len; i++) \
        (l)->cachedtks[i - 1] = (l)->cachedtks[i]; \
    (l)->cachedtk_len--; \
} while (0)


/* string interning */
static const char *internstr(lexer *l, const char *start, size_t len)
{
    struct h_entry *intern;
    intern = hashmap_searchlen(l->lexemes, start, len);
    if (nullhentry(intern)) {
        const char *dupped = string_duplen(start, len);
        /* TODO */
        /* index this to integer for constant table lookup in compiling */
        hashmap_insert(l->lexemes, dupped, (struct seal_value) {0});
        return dupped;
    } else {
        return intern->key;
    }
}

static struct token get_number(lexer *l)
{
    size_t len = 1;
    const char *start = &cur(l) - 1;
    int mode = NUM_DEC;

    if (*start == '0') {
        if (match(l, 'b') || match(l, 'B')) {
            mode = NUM_BIN;
            advance(l); len++;
            if (!isbin(cur(l)))
                lerror(l, "invalid binary literal");
        } else if (match(l, 'o') || match(l, 'O')) {
            mode = NUM_OCT;
            advance(l); len++;
            if (!isoct(cur(l)))
                lerror(l, "invalid octal literal");
        } else if (match(l, 'x') || match(l, 'X')) {
            mode = NUM_HEX;
            advance(l); len++;
            if (!ishex(cur(l)))
                lerror(l, "invalid hexadecimal literal");
        }
    } else if (*start == '.') {
        mode = NUM_FLT;
    }


    char c;
    while (!iseof(l)) {
        c = cur(l);
        switch (mode) {
        case NUM_DEC:
            if (!isnum(c)) {
                if (c == '.' && isnum(peek(l, 1))) {
                    mode = NUM_FLT;
                } else {
                    goto end;
                }
            }
            break;
        case NUM_BIN:
            if (!isbin(c))
                goto end;
            break;
        case NUM_OCT:
            if (!isoct(c))
                goto end;
            break;
        case NUM_HEX:
            if (!ishex(c))
                goto end;
            break;
        case NUM_FLT:
            if (!isnum(c))
                goto end;
            break;
        }
        advance(l); len++;
    }
end:
    return toklit(mode == NUM_FLT ? TK_FLOAT : TK_INT, internstr(l, start, len));
}

static const char *get_string(lexer *l, char cterm)
{
    size_t len = 0;
    const char *start = &cur(l);
    char *read, *write;
    read = write = (char *)&cur(l);
    while (!iseof(l) && !match(l, '\n') && !match(l, cterm)) {
        if (*read == '\\') {
            read++;
            advance(l);
            char esc;
            switch (*read) {
            case 'n': esc = '\n'; break;
            case 't': esc = '\t'; break;
            case '\\': esc = '\\'; break;
            case '\"': esc = '\"'; break;
            case '\'': esc = '\''; break;
            case 'r': esc = '\r'; break;
            case 'b': esc = '\b'; break;
            case 'f': esc = '\f'; break;
            case 'v': esc = '\v'; break;
            default:
                lerror(l, "unknown escape sequence: \'\\%c\'", *read);
                esc = '\0';
                break;
            }
            *write++ = esc;
            read++;
            len++;
            advance(l);
            continue;
        }
        *write++ = *read++;
        len++;
        advance(l);
    }

    if (!matchadv(l, cterm))
        lerror(l, "unterminated string");

    return internstr(l, start, len);
}

static token get_idtk(lexer *l)
{
    size_t len = 1;
    const char *start = &cur(l) - 1; /* for advancing one char before call */
    while (!iseof(l) && isidchr(cur(l))) {
        len++;
        advance(l);
    }

    if (len >= SHORTEST_TK_LEN && len <= LONGEST_TK_LEN) {
        for (int i = 0; i < NUM_WORD_TOKENS; i++) {
            if (strlen(token_names[i]) == len &&
                strncmp(start, token_names[i], len) == 0)
                return tokword(i + FIRST_WORD_TOKEN);
        }
    }

    return toklit(TK_ID, internstr(l, start, len));
}

static token nexttoken(lexer *l)
{
    char c;
    while (!iseof(l)) {
        c = advance(l);
        switch (c) {
        case '\n': case '\r': {
            newline(l);
            resetilvl(l);
            l->tkaftercom = false;
            if (!l->seen_word || isinparen(l)) {
                continue;
            }
            token t = TNEWLINE;
            t.line--;
            return t;
        }
        case ' ': case '\t':
            if (!l->seen_word)
                l->cur_indent++;
            continue;
        /* 
         * $ ! , . + - * / % ~ ^ | & ? : < > = ( ) [ ] { }
         */
        case '.': /* float or single dot */
            if (matchadv(l, '.')) {
                if (matchadv(l, '.'))
                    return tokword(TK_ELLIP);
                return tokword(TK_DPERIOD);
            }
            if (isnum(cur(l)))
                return get_number(l);
            return tokchar(c);
        case '=':
            return chr1or2tk(l, c, '=', TK_EQ);
        case '!':
            return chr1or2tk(l, c, '=', TK_NE);
        case '>':
            if (matchadv(l, '='))
                return tokword(TK_GE);
            if (matchadv(l, '>')) {
                if (matchadv(l, '='))
                    return tokword(TK_SHR_ASSIGN);
                return tokword(TK_SHR);
            }
            return tokchar(c);
        case '<':
            if (matchadv(l, '='))
                return tokword(TK_LE);
            if (matchadv(l, '<')) {
                if (matchadv(l, '='))
                    return tokword(TK_SHL_ASSIGN);
                return tokword(TK_SHL);
            }
            return tokchar(c);
        case '+':
            if (matchadv(l, '='))
                return tokword(TK_ADD_ASSIGN);
            if (matchadv(l, '+'))
                return tokword(TK_INC);
            return tokchar(c);
        case '-':
            if (matchadv(l, '='))
                return tokword(TK_SUB_ASSIGN);
            if (matchadv(l, '>'))
                return tokword(TK_ARROW);
            if (matchadv(l, '-'))
                return tokword(TK_DEC);
            return tokchar(c);
        case '*':
            return chr1or2tk(l, c, '=', TK_MUL_ASSIGN);
        case '/':
            if (matchadv(l, '/')) {
                skiplcom(l);
                continue;
            } else if (matchadv(l, '*')) {
                skipccom(l);
                continue;
            }
            return chr1or2tk(l, c, '=', TK_DIV_ASSIGN);
        case '%':
            return chr1or2tk(l, c, '=', TK_MOD_ASSIGN);
        case '^':
            return chr1or2tk(l, c, '=', TK_XOR_ASSIGN);
        case '|':
            return chr1or2tk(l, c, '=', TK_OR_ASSIGN);
        case '&':
            return chr1or2tk(l, c, '=', TK_AND_ASSIGN);
        case '(': case '[': case '{':
            parenpush(l, c);
            return tokchar(c);
        case ')': case ']': case '}':
            parenpop(l, c);
            return tokchar(c);
        case '\'': case '\"':
            return toklit(TK_STRING, get_string(l, c));
        case '$': case ',': case '~':
        case '?': case ':':
            return tokchar(c);
        default:
            if (isnum(c))
                return get_number(l);
            if (isidchr(c)) /* check this after number checking */
                return get_idtk(l);
            lerror(l, "unrecognized character: \'%c\'", c);
        }
    }
    if (isinparen(l)) {
        lerrorln(l,
                 l->parenline_stk[l->paren_count - 1],
                 "\'%c\' was never closed",
                 l->paren_stk[l->paren_count - 1]);
    }
    return TEOF;
}

static token handletoken(lexer *l)
{
    token t;

    if (hascache(l)) {
        popcachedtk(l, &t);
        return t;
    }

    t = nexttoken(l);

    if (t.type == TK_EOF) {
        while (l->indent_count > 1) {
            popilvl(l);
            cachetk(l, TDEDENT);
            cachetk(l, TNEWLINE);
        }
        if (l->tokcur.type != -1 && l->tokcur.type != TK_NEWLINE)
            return TNEWLINE;

        if (hascache(l))
            popcachedtk(l, &t);

        return t;
    } else {
        if (l->tkaftercom)
            lerror(l, "token after comment block not allowed");
    }

    if (t.type == TK_NEWLINE) {
        l->seen_word = false;
    } else {
        l->seen_word = true;

        if (isclosingparentk(t) || (isinparen(l) && !(isfirstparen(l) && isopenparentk(t))))
            return t;

        if (curilvl(l) > previlvl(l)) {
            pushilvl(l);
            cachetk(l, t);
            return TINDENT;
        } else if (curilvl(l) < previlvl(l)) {
            do {
                popilvl(l);
                cachetk(l, TDEDENT);
                cachetk(l, TNEWLINE);
            } while (hasilvl(l) && curilvl(l) != previlvl(l));
            cachetk(l, t);

            if (!hasilvl(l))
                lerror(l, "mismatch unindentation");

            popcachedtk(l, &t);
            return t;
        }
    }

    return t;
}

struct token lexer_get_token(lexer *l)
{
    struct token t = handletoken(l);
    return l->tokcur = t;
}

void lexer_reset(struct lexer *l)
{
    l->i = 0;
    l->line = l->col = 1;
    l->tokcur.type = -1; /* uninitialized */
    l->cur_indent = 0;
    l->paren_count = 0;
    l->indent_count = 1;
    l->cachedtk_len = 0;
    l->seen_word = false;
    l->indent_stk[0] = 0;
    l->seen_word = false;
    l->tkaftercom = false;
}

void lexer_init(struct lexer *l, const char *src)
{
    l->src = src;
    if (!l->lexemes)
        l->lexemes = hashmap_Nnew(LEXEME_START_CAP);
    lexer_reset(l);
}

const char *tkname(int type)
{
    if (type < FIRST_WORD_TOKEN) {
        return NULL;
    }
    return token_names[type - FIRST_WORD_TOKEN];
}
