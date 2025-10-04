#include "ast.h"

#if SEAL_DEBUG
#include <stdio.h>


#define TAB 4


#define print_tabs() (printf("%*s", i, ""))
#define print(...) ( \
    print_tabs(), \
    printf(__VA_ARGS__) \
)

static const char *const IMOP_NAMES[] = {
    ///* unaries */
    "<>++",//IMOP_POSTFIX_INC, /* a++ */
    "<>--",//IMOP_POSTFIX_DEC, /* a-- */
    "++<>",//IMOP_PREFIX_INC,  /* ++b */
    "--<>",//IMOP_PREFIX_DEC,  /* --a */
    "-",//IMOP_UNARY_MINUS, /* -a */
    "!",//IMOP_LOGICAL_NOT, /* !a (or) not a (even combined: !not!!not a) */
    "~",//IMOP_BITWISE_NOT, /* ~a */

    ///* binaries */
    "*", "/", "%",//IMOP_MUL, IMOP_DIV, IMOP_MOD, /* a * b / c % d */
    "+", "-",//IMOP_ADD, IMOP_SUB, /* a + b - c */
    "<<", ">>",//IMOP_SHL, IMOP_SHR, /* a << b >> c */
    "<", "<=",//IMOP_LT, IMOP_LE,   /* a < b <= c */
    ">", ">=",//IMOP_GT, IMOP_GE,   /* a > b >= c */
    "==", "!=",//IMOP_EQ, IMOP_NE,   /* a == b != c */
    "&",//IMOP_BITWISE_AND,   /* a & b */
    "^",//IMOP_XOR,           /* a ^ b */
    "|",//IMOP_BITWISE_OR,    /* a | b */

    ///* logical binaries */
    "and",//IMOP_AND, /* a and b */
    "or",//IMOP_OR,  /* a or b */

    ///* assignments */
    "=",//IMOP_ASSIGN, /* = */
    "*=", "/=", "%=",//IMOP_MUL_ASSIGN, IMOP_DIV_ASSIGN, IMOP_MOD_ASSIGN,
    "+=", "-=",//IMOP_ADD_ASSIGN, IMOP_SUB_ASSIGN,
    "<<=", ">>=",//IMOP_SHL_ASSIGN, IMOP_SHR_ASSIGN,
    "&=", "^=", "|=",//IMOP_AND_ASSIGN, IMOP_XOR_ASSIGN, IMOP_OR_ASSIGN,
};

#define imop_name(t) (IMOP_NAMES[(t)])

void dump_ast(struct ast *node, int i)
{
    switch (node->type) {
    case AST_NOP:
        print("NOP\n");
        break;
    case AST_NULL:
        print("null\n");
        break;
    case AST_TRUE:
        print("true\n");
        break;
    case AST_FALSE:
        print("false\n");
        break;
    case AST_INT:
        print("int: %lld\n", node->as.i);
        break;
    case AST_FLOAT:
        print("float: %lf\n", node->as.f);
        break;
    case AST_STRING:
        print("str: %s\n", node->as.s);
        break;
    case AST_LIST:
        print("list: size: %zu\n", node->as.l.size);
        while (node->as.l.items) {
            dump_ast(node->as.l.items, i + TAB);
            node->as.l.items = node->as.l.items->next;
        }
        break;
    case AST_MAP:
        print("map: size: %zu\n", node->as.m.size);
        while (node->as.m.pairs) {
            print_tabs();
            printf("key: %s, val:\n", node->as.m.pairs->as.pair.key);
            dump_ast(node->as.m.pairs->as.pair.val, i + TAB);
            node->as.m.pairs = node->as.m.pairs->next;
        }
        break;
    case AST_NAME:
        print("name: ");
        if (node->as.name.global)
            putchar('$');
        fputs(node->as.name.s, stdout);
        if (node->as.name.safe)
            putchar('?');
        putchar('\n');
        break;
    case AST_COMP:
        print("comp: size: %zu\n", node->as.comp.size);
        while (node->as.comp.stmts) {
            dump_ast(node->as.comp.stmts, i + TAB);
            node->as.comp.stmts = node->as.comp.stmts->next;
        }
        break;
    case AST_IF:
        print("if: haselse: %d, cond:\n", node->as.ifstmt.haselse);
        dump_ast(node->as.ifstmt.cond, i + TAB);
        print("if body:\n");
        dump_ast(node->as.ifstmt.body, i + TAB);
        if (node->as.ifstmt.haselse) {
            print("else part:\n");
            dump_ast(node->as.ifstmt.elsepart, i + TAB);
        }
        break;
    case AST_ELSE:
        print("else:\n");
        dump_ast(node->as.elsestmt.body, i + TAB);
        break;
    case AST_WHILE: case AST_DOWHILE:
        print("%swhile: cond:\n",
              node->type == AST_WHILE ? "" : "do");
        dump_ast(node->as.whilestmt.cond, i + TAB);
        print("while body:\n");
        dump_ast(node->as.whilestmt.body, i + TAB);
        break;
    case AST_FOR:
        print("for not defined\n");
        break;
    case AST_SKIP:
        print("skip\n");
        break;
    case AST_STOP:
        print("stop\n");
        break;
    case AST_RETURN:
        print("return: expr:\n");
        dump_ast(node->as.retstmt.val, i + TAB);
        break;
    case AST_INCLUDE:
        print("include \'%s\'", node->as.inclstmt.fname);
        break;
    case AST_TERNARY:
        print("ternary: cond:\n");
        dump_ast(node->as.ternary.c, i + TAB);
        print("ternary: then:\n");
        dump_ast(node->as.ternary.t, i + TAB);
        print("ternary: else:\n");
        dump_ast(node->as.ternary.e, i + TAB);
        break;
    case AST_LOGBIN:
        print("logical ");
    case AST_UNARY:
        print("unary: op: %s\n", imop_name(node->as.unary.op));
        print("unary expr:\n");
        dump_ast(node->as.unary.e, i + TAB);
        break;
    case AST_BINARY:
        print("binary: op: %s\n", imop_name(node->as.bin.op));
        print("binary left:\n");
        dump_ast(node->as.bin.l, i + TAB);
        print("binary right:\n");
        dump_ast(node->as.bin.r, i + TAB);
        break;
    case AST_FUNC_DEF:
        print("function: %s, size: %zu\n",
              node->as.func.name
              ? node->as.func.name
              : "<anonymous>",
              node->as.func.psize);
        print("parameters:\n");
        while (node->as.func.params) {
            dump_ast(node->as.func.params, i + TAB);
            node->as.func.params = node->as.func.params->next;
        }
        print("function body:\n");
        dump_ast(node->as.func.body, i + TAB);
        break;
    case AST_FUNC_CALL: case AST_METH_CALL:
        print("%s call: size: %zu, main:\n",
              node->type == AST_FUNC_CALL
              ? "function"
              : "method",
              node->as.call.argc);
        dump_ast(node->as.call.f, i + TAB);
        print("arguments:\n");
        while (node->as.call.argv) {
            dump_ast(node->as.call.argv, i + TAB);
            node->as.call.argv = node->as.call.argv->next;
        }
        break;
    case AST_INDEX:
        print("index: main:\n");
        dump_ast(node->as.index.m, i + TAB);
        print("index: index:\n");
        dump_ast(node->as.index.i, i + TAB);
        break;
    case AST_FIELD:
        print("field: main:\n");
        dump_ast(node->as.field.m, i + TAB);
        print("field: field:\n");
        dump_ast(node->as.field.f, i + TAB);
        break;
    default:
        print("define that first mf!\n");
        break;
    }
}

#endif
