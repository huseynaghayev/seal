#ifndef SEAL_AST_H
#define SEAL_AST_H

#include "sealconf.h"

#include "token.h"

/* datas */
#define AST_NOP           0
#define AST_NULL          1
#define AST_INT           2
#define AST_FLOAT         3
#define AST_STRING        4
#define AST_BOOL          5
#define AST_LIST          6
#define AST_MAP           7
#define AST_VAR_REF       8
#define AST_FUNC_CALL     9
#define AST_SUBSCRIPT     10
#define AST_MEMACC        11
/* blocks */
#define AST_COMP          13
#define AST_IF            14
#define AST_ELSE          15
#define AST_DOWHILE       16
#define AST_WHILE         17
#define AST_FOR           18
#define AST_FUNC_DEF      19
/* block control */
#define AST_SKIP          20
#define AST_STOP          21
#define AST_RETURN        22
/* operations */
#define AST_UNARY         23
#define AST_BINARY        24
#define AST_BINARY_BOOL   25
#define AST_TERNARY       26
#define AST_ASSIGN        27
/* others */
#define AST_INCLUDE       28
/* last index */
#define AST_LAST AST_INCLUDE

#define ast_type(ast) ast->type


typedef struct ast {
  int type;
  int line;

  union {
    /* datas */
    struct {
      seal_int val;
    } integer;
    struct {
      seal_float val;
    } floating;
    struct {
      char* val;
    } string;
    struct {
      bool val;
    } boolean;
    struct {
      size_t mem_size;
      struct ast** mems;
    } list;
    struct {
      const char** field_names;
      struct ast** field_vals;
      size_t field_size;
    } map;
    struct {
      const char* name;
      bool is_global;
    } var_ref;
    struct {
      struct ast* main;
      struct ast** args;
      size_t arg_size;
      bool is_method;
    } func_call;
    struct {
      struct ast* main;
      struct ast* index;
    } subscript;
    struct {
      struct ast* main;
      struct ast* mem;
    } memacc;
    /* blocks */
    struct {
      struct ast** stmts;
      size_t stmt_size;
    } comp;
    struct {
      struct ast* cond;
      struct ast* comp;
      struct ast* _else;
      bool has_else;
    } _if;
    struct {
      struct ast* comp;
    } _else;
    struct {
      struct ast* cond;
      struct ast* comp;
    } _while;
    struct {
      const char* it_name;
      struct ast* ited;
      struct ast* comp;
    } _for;
    struct {
      const char* name;
      const char** param_names;
      size_t param_size;
      bool is_variadic;
      struct ast* comp;
    } func_def;
    /* block control */
    struct {
      struct ast* expr;
    } _return;
    /* operations */
    struct {
      struct ast* expr;
      int op_type;
      enum {
        PREFIX,
        POSTFIX
      } type;
    } unary;
    struct {
      struct ast* left;
      struct ast* right;
      int op_type;
    } binary;
    struct {
      struct ast* cond;
      struct ast* expr_true;
      struct ast* expr_false;
    } ternary;
    struct {
      struct ast* var;
      struct ast* expr;
      int op_type;
    } assign;
    /* others */
    struct {
      const char* name;
      const char* alias;
      const char **symbols;
      size_t symbols_size;
    } include;
  };
} ast_t;

static inline const char* ast_type_name(int type)
{
  switch (type) {
    case AST_NOP          : return "AST_NOP";
    case AST_NULL         : return "AST_NULL";
    case AST_INT          : return "AST_INT";
    case AST_FLOAT        : return "AST_FLOAT";
    case AST_STRING       : return "AST_STRING";
    case AST_BOOL         : return "AST_BOOL";
    case AST_LIST         : return "AST_LIST";
    case AST_MAP          : return "AST_MAP";
    case AST_VAR_REF      : return "AST_VAR_REF";
    case AST_FUNC_CALL    : return "AST_FUNC_CALL";
    case AST_SUBSCRIPT    : return "AST_SUBSCRIPT";
    case AST_MEMACC       : return "AST_MEMACC";
    case AST_COMP         : return "AST_COMP";
    case AST_IF           : return "AST_IF";
    case AST_ELSE         : return "AST_ELSE";
    case AST_DOWHILE      : return "AST_DOWHILE";
    case AST_WHILE        : return "AST_WHILE";
    case AST_FOR          : return "AST_FOR";
    case AST_FUNC_DEF     : return "AST_FUNC_DEF";
    case AST_SKIP         : return "AST_SKIP";
    case AST_STOP         : return "AST_STOP";
    case AST_RETURN       : return "AST_RETURN";
    case AST_UNARY        : return "AST_UNARY";
    case AST_BINARY       : return "AST_BINARY";
    case AST_BINARY_BOOL  : return "AST_BINARY_BOOL";
    case AST_TERNARY      : return "AST_TERNARY";
    case AST_ASSIGN       : return "AST_ASSIGN";
    case AST_INCLUDE      : return "AST_INCLUDE";
    default               : return "AST NOT RECOGNIZED";
  }
}

static inline const char* hast_type_name(int type)
{
  switch (type) {
    case AST_NOP          : return "no operation";
    case AST_NULL         : return "null";
    case AST_INT          : return "int";
    case AST_FLOAT        : return "float";
    case AST_STRING       : return "string";
    case AST_BOOL         : return "bool";
    case AST_LIST         : return "list";
    case AST_MAP          : return "map";
    case AST_VAR_REF      : return "variable reference";
    case AST_FUNC_CALL    : return "function call";
    case AST_SUBSCRIPT    : return "subscript";
    case AST_MEMACC       : return "member access";
    case AST_COMP         : return "compound";
    case AST_IF           : return "if";
    case AST_ELSE         : return "else";
    case AST_DOWHILE      : return "do while";
    case AST_WHILE        : return "while";
    case AST_FOR          : return "for";
    case AST_FUNC_DEF     : return "function definition";
    case AST_SKIP         : return "skip";
    case AST_STOP         : return "stop";
    case AST_RETURN       : return "return";
    case AST_UNARY        : return "unary";
    case AST_BINARY       : return "binary";
    case AST_BINARY_BOOL  : return "binary bool";
    case AST_TERNARY      : return "ternary";
    case AST_ASSIGN       : return "assignment";
    case AST_INCLUDE      : return "include";
    default               : return "<AST NAME NOT FOUND>";
  }
}

static inline ast_t* create_ast(int type)
{
  ast_t* ast = (ast_t*)SEAL_CALLOC(1, sizeof(ast_t));
  
  ast->type = type;
  ast->line = 0;

  return ast;
}

static inline ast_t* static_create_ast(int type, int line)
{
  ast_t* ast = create_ast(type);
  
  ast->line = line;

  return ast;
}

void create_const_asts(); /* this function must be called only once */

/* constant nodes are called with these functions */
ast_t* ast_nop();
ast_t* ast_null();
ast_t* ast_true();
ast_t* ast_false();

static void print_ast(ast_t* node)
{
  switch (node->type) {
    /* datas */
    case AST_NOP:
    case AST_NULL:
      printf("%d: %s\n",
              node->line,
              hast_type_name(node->type));
      break;
    case AST_INT:
      printf("%d: %s: \'%lld\'\n",
              node->line,
              hast_type_name(node->type),
              node->integer.val);
      break;
    case AST_FLOAT:
      printf("%d: %s: \'%f\'\n",
              node->line,
              hast_type_name(node->type),
              node->floating.val);
      break;
    case AST_STRING:
      printf("%d: %s: \'%s\'\n",
              node->line,
              hast_type_name(node->type),
              node->string.val);
      break;
    case AST_BOOL:
      printf("%d: %s: \'%s\'\n",
              node->line,
              hast_type_name(node->type),
              node->boolean.val ? "true" : "false");
      break;
    case AST_LIST:
      printf("%d: %s: size: %zu, members:\n",
              node->line,
              hast_type_name(node->type),
              node->list.mem_size);
      for (int i = 0; i < node->list.mem_size; i++) {
        printf("\tmem %d:\n", i);
        print_ast(node->list.mems[i]);
      }
      break;
    case AST_MAP:
      printf("%d: %s: size: %zu, fields:\n",
              node->line,
              hast_type_name(node->type),
              node->map.field_size);
      for (int i = 0; i < node->map.field_size; i++) {
        printf("\t");
        printf("field %s:\n", node->map.field_names[i]);
        printf("\t");
        print_ast(node->map.field_vals[i]);
      }
      break;
    case AST_VAR_REF:
      printf("%d: %s%s: \'%s\'\n",
              node->line,
              node->var_ref.is_global ? "global " : "",
              hast_type_name(node->type),
              node->var_ref.name);
      break;
    case AST_FUNC_CALL:
      printf("%d: %s: is method: %d, arg size: %zu%s\n",
              node->line,
              hast_type_name(node->type),
              node->func_call.is_method,
              node->func_call.arg_size,
              node->func_call.arg_size > 0 ? ", args:" : "");
      for (int i = 0; i < node->func_call.arg_size; i++) {
        printf("\targ %d:\n", i);
        print_ast(node->func_call.args[i]);
      }
      printf("callee:\n");
      print_ast(node->func_call.main);
      break;
    case AST_SUBSCRIPT:
      printf("%d: %s: main:\n",
              node->line,
              hast_type_name(node->type));
      printf("\t");
      print_ast(node->subscript.main);
      printf("%d: %s: index:\n",
              node->line,
              hast_type_name(node->type));
      printf("\t");
      print_ast(node->subscript.index);
      break;
    case AST_MEMACC:
      printf("%d: %s: main:\n",
              node->line,
              hast_type_name(node->type));
      printf("\t");
      print_ast(node->memacc.main);
      printf("%d: %s: member:\n",
              node->line,
              hast_type_name(node->type));
      printf("\t");
      print_ast(node->memacc.mem);
      break;
    /* blocks */
    case AST_COMP:
      printf("%d: %s: size: %zu\n",
              node->line,
              hast_type_name(node->type),
              node->comp.stmt_size);
      for (int i = 0; i < node->comp.stmt_size; i++) {
        printf("comp node %d:\n", i);
        print_ast(node->comp.stmts[i]);
      }
      break;
    case AST_IF:
      printf("%d: %s: has else: %d, condition:\n",
              node->line,
              hast_type_name(node->type),
              node->_if.has_else);
      print_ast(node->_if.cond);
      printf("body:\n");
      print_ast(node->_if.comp);
      if (node->_if.has_else) {
        printf("else part:\n");
        print_ast(node->_if._else);
      }
      break;
    case AST_ELSE:
      printf("%d: %s, body:\n",
              node->line,
              hast_type_name(node->type));
      print_ast(node->_else.comp);
      break;
    case AST_DOWHILE:
    case AST_WHILE:
      printf("%d: %s: condition:\n",
              node->line,
              hast_type_name(node->type));
      print_ast(node->_while.cond);
      printf("body:\n");
      print_ast(node->_while.comp);
      break;
    case AST_FOR:
      printf("%d: %s: iterator name: %s, iterated:\n",
                node->line,
                hast_type_name(node->type),
                node->_for.it_name);
      print_ast(node->_for.ited);
      printf("body:\n");
      print_ast(node->_for.comp);
      break;
    case AST_FUNC_DEF:
      printf("%d: %s: \'%s\', is variadic: %d, parameter size: %zu%s\n",
              node->line,
              hast_type_name(node->type),
              node->func_def.name,
              node->func_def.is_variadic,
              node->func_def.param_size,
              node->func_def.param_size > 0 ? ", parameters:" : "");
      for (int i = 0; i < node->func_def.param_size; i++) {
        printf("\tparameter %d: \'%s\'\n", i, node->func_def.param_names[i]);
      }
      printf("function body:\n");
      print_ast(node->func_def.comp);
      break;
    /* block control */
    case AST_SKIP:
      printf("%d: %s\n",
              node->line,
              hast_type_name(node->type));
      break;
    case AST_STOP:
      printf("%d: %s\n",
              node->line,
              hast_type_name(node->type));
      break;
    case AST_RETURN:
      printf("%d: %s, expr:\n",
              node->line,
              hast_type_name(node->type));
      printf("\t");
      print_ast(node->_return.expr);
      break;
    /* operations */
    case AST_UNARY:
      printf("%d: %s, op: \'%s\'%s, expr:\n",
              node->line,
              hast_type_name(node->type),
              htoken_type_name(node->unary.op_type),
              node->unary.op_type == TOK_INC || node->unary.op_type == TOK_DEC ?
                node->unary.type == POSTFIX ? " postfix" : " prefix"
                : "");
      print_ast(node->unary.expr);
      break;
    case AST_BINARY:
      printf("%d: %s, op: \'%s\',\nleft side:\n",
              node->line,
              hast_type_name(node->type),
              htoken_type_name(node->binary.op_type));
      print_ast(node->binary.left);
      printf("right side:\n");
      print_ast(node->binary.right);
      break;
    case AST_BINARY_BOOL:
      printf("%d: %s, op: \'%s\',\nleft side:\n",
              node->line,
              hast_type_name(node->type),
              htoken_type_name(node->binary.op_type));
      print_ast(node->binary.left);
      printf("right side:\n");
      print_ast(node->binary.right);
      break;
    case AST_TERNARY:
      printf("%d: %s, condition:\n",
              node->line,
              hast_type_name(node->type));
      print_ast(node->ternary.cond);
      printf("true side:\n");
      print_ast(node->ternary.expr_true);
      printf("false side:\n");
      print_ast(node->ternary.expr_false);
      break;
    case AST_ASSIGN:
      printf("%d: %s: op: %s, variable:\n",
              node->line,
              htoken_type_name(node->assign.op_type),
              hast_type_name(node->type));
      print_ast(node->assign.var);
      printf("assigned expression:\n");
      print_ast(node->assign.expr);
      break;
    /* others */
    case AST_INCLUDE:
      printf("%d: %s: \'%s\' %s\n",
              node->line,
              hast_type_name(node->type),
              node->include.name,
              node->include.alias ? node->include.alias : "");
      break;
    default:
      fprintf(stderr, "unexpected AST type: \'%d\'\n", node->type);
  }
}

#endif /* SEAL_AST_H */
