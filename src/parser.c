#include "parser.h"
#include "ast.h"
#include <stdbool.h>

#define parser_is_end(parser) ( \
  parser->i >= parser->tok_size)

#define parser_eof_index(parser) ( \
  parser->tok_size - 1)

#define parser_eof(parser) ( \
  parser->toks[parser->tok_size - 1])

#define parser_peek(parser) ( \
  parser->toks[parser->i])

#define parser_match(parser, t) ( \
  parser_peek(parser)->type == t)

#define parser_line(parser) ( \
  parser_peek(parser)->line)

#define parser_peek_offset(parser, offset) ( \
  (parser->i + (offset) >= 0 && parser->i + (offset) < parser->tok_size)\
  ? parser->toks[parser->i + (offset)]\
  : parser_eof(parser))

#define is_lvalue(node) ( \
  node->type == AST_VAR_REF || \
  node->type == AST_SUBSCRIPT || \
  node->type == AST_MEMACC)
  
static inline ast_t* parser_error(parser_t* parser, const char* err)
{
  fprintf(stderr, "seal: file: \'%s\', line %d\nsyntax error: %s\n", parser->file_path, parser_line(parser), err);
  exit(1);
  return ast_nop();
}

static inline void kill_if_duplicated_name(parser_t* parser,
                                           const char* name,
                                           const char** names,
                                           size_t name_size)
{
  for (int i = 0; i < name_size; i++) {
    if (strcmp(name, names[i]) == 0) {
      char err[ERR_LEN];
      sprintf(err, "duplicate parameter: \'%s\' used", name);
      parser_error(parser, err);
    }
  }
}

static inline token_t* parser_advance(parser_t* parser)
{
  return parser->toks[parser_is_end(parser) ? parser_eof_index(parser) : parser->i++];
}

static inline token_t* parser_eat(parser_t* parser, int type)
{
  if (parser_match(parser, type)) return parser_advance(parser);
  if (type == TOK_DEDENT && parser_match(parser, TOK_EOF)) return parser_peek(parser);

  int prev_tok_type = parser_peek_offset(parser, -1)->type;
  bool followed_word = prev_tok_type != TOK_NEWL &&
                       prev_tok_type != TOK_INDENT &&
                       prev_tok_type != TOK_DEDENT &&
                       prev_tok_type != TOK_EOF;

  char err[ERR_LEN];
  sprintf(err,
          "expected \'%s\' instead of \'%s\'",
          htoken_type_name(type),
          parser_peek(parser)->val);
  if (followed_word) {
    char after[ERR_LEN];
    sprintf(after, " after \'%s\'", parser_peek_offset(parser, -1)->val);
    strcat(err, after);
  }

  parser_error(parser, err);
  return NULL;
}

/* main function */
inline void init_parser(parser_t* parser, lexer_t* lexer)
{
  parser->file_path = lexer->file_path;
  parser->toks = lexer->toks;
  parser->tok_size = lexer->tok_size;
  parser->i = 0;
}

inline ast_t* parser_parse(parser_t* parser)
{
  return parser_parse_statements(parser, false, false, false);
}

/* parsing statement */
static ast_t* parser_parse_statements(parser_t* parser,
                                      bool is_func,
                                      bool is_ifelse,
                                      bool is_loop)
{
  ast_t* ast = static_create_ast(AST_COMP, parser_line(parser));

  ast->comp.stmt_size = 1;
  ast->comp.stmts = SEAL_CALLOC(1, sizeof(ast_t*));
  ast->comp.stmts[0] = parser_parse_statement(parser, is_func, is_ifelse, is_loop, false);

  while (!parser_is_end(parser) && parser_match(parser, TOK_NEWL)) {
    parser_eat(parser, TOK_NEWL);

    ast->comp.stmt_size++;
    ast->comp.stmts = SEAL_REALLOC(ast->comp.stmts, ast->comp.stmt_size * sizeof(ast_t*));
    ast->comp.stmts[ast->comp.stmt_size - 1] = parser_parse_statement(parser, is_func, is_ifelse, is_loop, false);
  }

  bool is_global_scope = !is_func && !is_ifelse && !is_loop;
  if (is_global_scope && !parser_match(parser, TOK_EOF)) {
    char err[ERR_LEN];
    sprintf(err, "unexpected \'%s\' after \'%s\'", parser_peek(parser)->val, parser_peek_offset(parser, -1)->val);
    return parser_error(parser, err);
  } else if (!is_global_scope && !parser_match(parser, TOK_DEDENT)) {
    char err[ERR_LEN];
    sprintf(err, "unexpected \'%s\' after \'%s\'", parser_peek(parser)->val, parser_peek_offset(parser, -1)->val);
    return parser_error(parser, err);
  }

  return ast;
}

static ast_t* parser_parse_statement(parser_t* parser,
                                     bool is_func,
                                     bool is_ifelse,
                                     bool is_loop,
                                     bool is_inline)
{
  bool is_global_scope = !is_func && !is_ifelse && !is_loop;
  switch (parser_peek(parser)->type) {
    case TOK_IF:
      if (is_inline)
        return parser_parse_ternary(parser);
      return parser_parse_if(parser, true, is_func, is_loop);
    case TOK_DO:
      if (is_inline) goto error;
      return parser_parse_dowhile(parser, is_func);
    case TOK_WHILE:
      if (is_inline) goto error;
      return parser_parse_while(parser, is_func);
    case TOK_FOR:
      if (is_inline) goto error;
      return parser_parse_for(parser, is_func);
    case TOK_DEFINE:
      return parser_parse_func_def(parser, true /* can be global */);
    case TOK_SKIP:
      if (!is_loop) goto error;
      return parser_parse_skip(parser);
    case TOK_STOP:
      if (!is_loop) goto error;
      return parser_parse_stop(parser);
    case TOK_RETURN:
      if (!is_func) goto error;
      return parser_parse_return(parser);
    case TOK_INCLUDE:
      if (!is_global_scope) goto error;
      return parser_parse_include(parser);
    case TOK_DEDENT:
      if (is_global_scope) goto error;
      return ast_nop();
    case TOK_EOF:
      if (!is_global_scope) goto error;
      return ast_nop();
    default:
      return parser_parse_expr(parser);
  }
  error: {
    char err[ERR_LEN];
    sprintf(err, "unexpected \'%s\'", parser_peek(parser)->val);
    return parser_error(parser, err);
  }
}
static inline ast_t* parser_parse_inline_statement(parser_t* parser, bool is_func, bool is_ifelse, bool is_loop)
{
  ast_t* comp = static_create_ast(AST_COMP, parser_line(parser));
  comp->comp.stmt_size = 1;
  comp->comp.stmts = SEAL_CALLOC(1, sizeof(ast_t));
  comp->comp.stmts[0] = parser_parse_statement(parser, is_func, is_ifelse, is_loop, true);

  return comp;
}

/* parse datas */
static inline ast_t* parser_parse_int(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_INT, parser_line(parser));
  ast->integer.val = atol(parser_eat(parser, TOK_INT)->val);
  return ast;
}

static inline ast_t* parser_parse_float(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_FLOAT, parser_line(parser));
  ast->floating.val = atof(parser_eat(parser, TOK_FLOAT)->val);
  return ast;
}

static inline ast_t* parser_parse_string(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_STRING, parser_line(parser));
  ast->string.val = (char*)parser_eat(parser, TOK_STRING)->val;
  return ast;
}

static ast_t* parser_parse_list(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_LIST, parser_line(parser));
  ast->list.mem_size = 0;
  ast->list.mems = NULL;

  parser_eat(parser, TOK_LBRACK);
  if (!parser_match(parser, TOK_RBRACK)) {
    ast->list.mem_size = 1;
    ast->list.mems = SEAL_CALLOC(1, sizeof(ast_t*));
    ast->list.mems[0] = parser_parse_expr(parser);
  }
  while (parser_match(parser, TOK_COMMA)) {
    parser_eat(parser, TOK_COMMA);

    ast->list.mem_size++;
    ast->list.mems = SEAL_REALLOC(ast->list.mems, ast->list.mem_size * sizeof(ast_t*));
    ast->list.mems[ast->list.mem_size - 1] = parser_parse_expr(parser);
  }
  parser_eat(parser, TOK_RBRACK);

  return ast;
}

static ast_t* parser_parse_map(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_MAP, parser_line(parser));
  ast->map.field_size = 0;
  ast->map.field_names = NULL;
  ast->map.field_vals = NULL;

  parser_eat(parser, TOK_LBRACE);
  if (!parser_match(parser, TOK_RBRACE)) {
    ast->map.field_size = 1;
    // field names
    ast->map.field_names = SEAL_CALLOC(1, sizeof(char*));
    ast->map.field_names[0] = parser_eat(parser, TOK_ID)->val;

    parser_eat(parser, TOK_ASSIGN); // require '='
    // field vals
    ast->map.field_vals = SEAL_CALLOC(1, sizeof(ast_t*));
    ast->map.field_vals[0] = parser_parse_expr(parser);
  }
  while (parser_match(parser, TOK_COMMA)) {
    parser_eat(parser, TOK_COMMA);

    // no duplicate allowed
    const char* field_name = parser_eat(parser, TOK_ID)->val;
    kill_if_duplicated_name(parser, field_name, ast->map.field_names, ast->map.field_size);

    ast->map.field_size++;
    // field names
    ast->map.field_names = SEAL_REALLOC(ast->map.field_names, ast->map.field_size * sizeof(char*));
    ast->map.field_names[ast->map.field_size - 1] = field_name;

    parser_eat(parser, TOK_ASSIGN); // require '='
    // field vals
    ast->map.field_vals = SEAL_REALLOC(ast->map.field_vals, ast->map.field_size * sizeof(ast_t*));
    ast->map.field_vals[ast->map.field_size - 1] = parser_parse_expr(parser);
  }
  parser_eat(parser, TOK_RBRACE);

  return ast;
}

static inline ast_t* parser_parse_id(parser_t* parser)
{
  return parser_parse_var_ref(parser);
}
static inline ast_t* parser_parse_var_ref(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_VAR_REF, parser_line(parser));
  ast->var_ref.name = parser_eat(parser, TOK_ID)->val;

  ast->var_ref.is_global = false; /* variables are local by default */

  return ast;
}
static ast_t* parser_parse_func_call(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_FUNC_CALL, parser_line(parser));

  ast->func_call.arg_size = 0;
  parser_eat(parser, TOK_LPAREN);

  if (!parser_match(parser, TOK_RPAREN)) {
    ast->func_call.arg_size = 1;
    ast->func_call.args = SEAL_CALLOC(1, sizeof(ast_t*));
    ast->func_call.args[0] = parser_parse_expr(parser);
  }

  while (!parser_match(parser, TOK_RPAREN)) {
    parser_eat(parser, TOK_COMMA);

    ast->func_call.arg_size++;
    ast->func_call.args = SEAL_REALLOC(ast->func_call.args, ast->func_call.arg_size * sizeof(ast_t*));
    ast->func_call.args[ast->func_call.arg_size - 1] = parser_parse_expr(parser);
  }

  parser_eat(parser, TOK_RPAREN);

  return ast;
}

/* parse blocks */
static ast_t* parser_parse_if(parser_t* parser, bool can_be_ternary, bool is_func, bool is_loop)
{
  if (can_be_ternary) {
    for (int i = 0; !parser_is_end(parser); i++) {
      token_t* tok = parser_peek_offset(parser, i);
      if (tok->type == TOK_NEWL) break;
      if (tok->type == TOK_ELSE) {
        return parser_parse_ternary(parser);
      }
    }
  }

  parser_eat(parser, TOK_IF);

  ast_t* ast = static_create_ast(AST_IF, parser_line(parser));
  ast->_if.has_else = false;

  ast->_if.cond = parser_parse_expr(parser);

  if (parser_match(parser, TOK_THEN)) { // inline if
    parser_advance(parser); // 'then'
    ast->_if.comp = parser_parse_inline_statement(parser, is_func, true, is_loop);
  } else {
    parser_eat(parser, TOK_NEWL);
    parser_eat(parser, TOK_INDENT);

    ast->_if.comp = parser_parse_statements(parser, is_func, true, is_loop);

    parser_eat(parser, TOK_DEDENT);
  }
  
  if (parser_peek_offset(parser, 1)->type == TOK_ELSE) {
    parser_eat(parser, TOK_NEWL);
    ast->_if.has_else = true;
    if (parser_peek_offset(parser, 1)->type == TOK_IF) {
      parser_advance(parser); // 'else'
      ast->_if._else = parser_parse_if(parser, false, is_func, is_loop);
    } else {
      ast->_if._else = parser_parse_else(parser, is_func, is_loop);
    }
  }

  return ast;
}

static ast_t* parser_parse_else(parser_t* parser, bool is_func, bool is_loop)
{
  ast_t* ast = static_create_ast(AST_ELSE, parser_line(parser));

  parser_eat(parser, TOK_ELSE);
  if (parser_match(parser, TOK_NEWL)) {
    parser_advance(parser); // newl
    parser_eat(parser, TOK_INDENT);

    ast->_else.comp = parser_parse_statements(parser, is_func, true, is_loop);

    parser_eat(parser, TOK_DEDENT);
  } else { // inline
    ast->_else.comp = parser_parse_inline_statement(parser, is_func, true, is_loop);
  }

  return ast;
}
static ast_t* parser_parse_dowhile(parser_t* parser, bool is_func)
{
  ast_t* ast = static_create_ast(AST_DOWHILE, parser_line(parser));

  parser_eat(parser, TOK_DO);
  if (parser_match(parser, TOK_NEWL)) {
    parser_advance(parser); // newl
    parser_eat(parser, TOK_INDENT);

    ast->_while.comp = parser_parse_statements(parser, is_func, false, true);

    parser_eat(parser, TOK_DEDENT);
    parser_eat(parser, TOK_NEWL);
    parser_eat(parser, TOK_WHILE);
    ast->_while.cond = parser_parse_expr(parser);
  } else {
    ast->_while.comp = parser_parse_inline_statement(parser, is_func, false, true);
    parser_eat(parser, TOK_WHILE);
    ast->_while.cond = parser_parse_expr(parser);
  }

  return ast;
}
static ast_t* parser_parse_while(parser_t* parser, bool is_func)
{
  parser_eat(parser, TOK_WHILE);

  ast_t* ast = static_create_ast(AST_WHILE, parser_line(parser));

  ast->_while.cond = parser_parse_expr(parser);

  if (parser_match(parser, TOK_DO)) { // inline while
    parser_advance(parser); // 'do'
    ast->_while.comp = parser_parse_inline_statement(parser, is_func, false, true);
  } else {
    parser_eat(parser, TOK_NEWL);
    parser_eat(parser, TOK_INDENT);

    ast->_while.comp = parser_parse_statements(parser, is_func, false, true);

    parser_eat(parser, TOK_DEDENT);
  }

  return ast;
}
static ast_t* parser_parse_for(parser_t* parser, bool is_func)
{
  parser_eat(parser, TOK_FOR);

  ast_t* ast = static_create_ast(AST_FOR, parser_line(parser));

  ast->_for.it_name = parser_eat(parser, TOK_ID)->val;
  parser_eat(parser, TOK_IN);

  ast->_for.ited = parser_parse_expr(parser);

  if (parser_match(parser, TOK_DO)) { // inline for
    parser_advance(parser); // 'do'
    ast->_for.comp = parser_parse_inline_statement(parser, is_func, false, true);
  } else {
    parser_eat(parser, TOK_NEWL);
    parser_eat(parser, TOK_INDENT);

    ast->_for.comp = parser_parse_statements(parser, is_func, false, true);

    parser_eat(parser, TOK_DEDENT);
  }

  return ast;
}
static ast_t* parser_parse_func_def(parser_t* parser, bool can_be_global)
{
  parser_eat(parser, TOK_DEFINE);

  ast_t* ast = static_create_ast(AST_FUNC_DEF, parser_line(parser));
  ast->func_def.param_size = 0;

  bool is_anonym   = true;
  bool is_variadic = false;
  if (can_be_global && parser_match(parser, TOK_ID))
    is_anonym = false;

  ast->func_def.name = is_anonym ? NULL : parser_eat(parser, TOK_ID)->val;

  parser_eat(parser, TOK_LPAREN);

  if (!parser_match(parser, TOK_RPAREN)) {
    if (parser_match(parser, TOK_ELLIP)) {
      parser_advance(parser);
      is_variadic = true;
    }

    ast->func_def.param_size = 1;
    ast->func_def.param_names = SEAL_CALLOC(1, sizeof(char*));
    ast->func_def.param_names[0] = parser_eat(parser, TOK_ID)->val;
  }

  while (!is_variadic && !parser_match(parser, TOK_RPAREN)) {
    parser_eat(parser, TOK_COMMA); // ',' separator

    if (parser_match(parser, TOK_ELLIP)) {
      parser_advance(parser);
      is_variadic = true;
    }

    const char* param = parser_eat(parser, TOK_ID)->val;
    kill_if_duplicated_name(parser, param, ast->func_def.param_names, ast->func_def.param_size);

    ast->func_def.param_size++;
    ast->func_def.param_names = SEAL_REALLOC(ast->func_def.param_names, ast->func_def.param_size * sizeof(char*));
    ast->func_def.param_names[ast->func_def.param_size - 1] = param;
  }

  ast->func_def.is_variadic = is_variadic;
  parser_eat(parser, TOK_RPAREN);

  if (parser_match(parser, TOK_NEWL)) {
    parser_eat(parser, TOK_NEWL);
    parser_eat(parser, TOK_INDENT);

    ast->func_def.comp = parser_parse_statements(parser, true, false, false);

    parser_eat(parser, TOK_DEDENT);
  } else {
    ast_t* ret_stmt = static_create_ast(AST_RETURN, parser_line(parser));
    ret_stmt->_return.expr = parser_parse_expr(parser);
    ast->func_def.comp = ret_stmt;
  }

  return ast;
}
/* parse block control */
static inline ast_t* parser_parse_skip(parser_t* parser)
{
  parser_advance(parser); // 'skip'
  ast_t* ast = static_create_ast(AST_SKIP, parser_line(parser));
  return ast;
}
static inline ast_t* parser_parse_stop(parser_t* parser)
{
  parser_advance(parser); // 'stop'
  ast_t* ast = static_create_ast(AST_STOP, parser_line(parser));
  return ast;
}
static inline ast_t* parser_parse_return(parser_t* parser)
{
  parser_advance(parser); // 'return'
  ast_t* ast = static_create_ast(AST_RETURN, parser_line(parser));
  ast->_return.expr = parser_match(parser, TOK_NEWL) ? ast_null() : parser_parse_expr(parser);
  return ast;
}
/* parse operations & expressions */
static inline ast_t* parser_parse_expr(parser_t* parser)
{
  return parser_match(parser, TOK_DEFINE) ?
           parser_parse_func_def(parser, false) :
         parser_match(parser, TOK_IF) ?
           parser_parse_ternary(parser) : parser_parse_or(parser);
}
static ast_t* parser_parse_ternary(parser_t* parser)
{
  ast_t* ast = static_create_ast(AST_TERNARY, parser_line(parser));
  parser_eat(parser, TOK_IF);
  ast->ternary.cond = parser_parse_expr(parser);
  parser_eat(parser, TOK_THEN);
  ast->ternary.expr_true = parser_parse_expr(parser);
  parser_eat(parser, TOK_ELSE);
  ast->ternary.expr_false = parser_parse_expr(parser);
  return ast;
}
static ast_t* parser_parse_or(parser_t* parser)
{
  ast_t* left = parser_parse_and(parser);

  while (parser_match(parser, TOK_OR)) {
    ast_t* bin = static_create_ast(AST_BINARY_BOOL, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_and(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_and(parser_t* parser)
{
  ast_t* left = parser_parse_in(parser);

  while (parser_match(parser, TOK_AND)) {
    ast_t* bin = static_create_ast(AST_BINARY_BOOL, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_in(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_in(parser_t* parser)
{
  ast_t* left = parser_parse_bor(parser);

  while (parser_match(parser, TOK_IN)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_bor(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_bor(parser_t* parser)
{
  ast_t* left = parser_parse_xor(parser);

  while (parser_match(parser, TOK_BOR)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_xor(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_xor(parser_t* parser)
{
  ast_t* left = parser_parse_band(parser);

  while (parser_match(parser, TOK_XOR)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_band(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_band(parser_t* parser)
{
  ast_t* left = parser_parse_equal(parser);

  while (parser_match(parser, TOK_BAND)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_equal(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_equal(parser_t* parser)
{
  ast_t* left = parser_parse_compare(parser);

  while (parser_match(parser, TOK_EQ) ||
         parser_match(parser, TOK_NE)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_compare(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_compare(parser_t* parser)
{
  ast_t* left = parser_parse_shift(parser);

  while (parser_match(parser, TOK_GT) ||
         parser_match(parser, TOK_GE) ||
         parser_match(parser, TOK_LT) ||
         parser_match(parser, TOK_LE)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_shift(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_shift(parser_t* parser)
{
  ast_t* left = parser_parse_term(parser);

  while (parser_match(parser, TOK_SHL) ||
         parser_match(parser, TOK_SHR)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_factor(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_term(parser_t* parser)
{
  ast_t* left = parser_parse_factor(parser);

  while (parser_match(parser, TOK_PLUS) ||
         parser_match(parser, TOK_MINUS)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_factor(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_factor(parser_t* parser)
{
  ast_t* left = parser_parse_unary(parser);

  while (parser_match(parser, TOK_MUL) ||
         parser_match(parser, TOK_DIV) ||
         parser_match(parser, TOK_MOD)) {
    ast_t* bin = static_create_ast(AST_BINARY, parser_line(parser));
    bin->binary.left = left;
    bin->binary.op_type = parser_advance(parser)->type; // optype
    bin->binary.right = parser_parse_unary(parser);
    left = bin;
  }

  return left;
}
static ast_t* parser_parse_unary(parser_t* parser)
{
  if (parser_match(parser, TOK_INC) ||
      parser_match(parser, TOK_DEC)) {
    ast_t* ast = static_create_ast(AST_UNARY, parser_line(parser));
    ast->unary.op_type = parser_advance(parser)->type;
    ast->unary.type = PREFIX;
    ast_t* expr = parser_parse_primary(parser);

    if (!is_lvalue(expr)) {
      char err[ERR_LEN];
      sprintf(err, "\'%s\' requires assignable value", htoken_type_name(ast->unary.op_type));
      parser_error(parser, err);
    }

    ast->unary.expr = expr;

    return ast;
  }
  if (parser_match(parser, TOK_NOT) ||
      parser_match(parser, TOK_MINUS) ||
      parser_match(parser, TOK_PLUS) ||
      parser_match(parser, TOK_TYPEOF) ||
      parser_match(parser, TOK_BNOT)) {
    ast_t* ast = static_create_ast(AST_UNARY, parser_line(parser));
    ast->unary.op_type = parser_advance(parser)->type; // optype
    ast->unary.expr = parser_parse_unary(parser);
    return ast;
  }
  return parser_parse_primary(parser);
}
static ast_t* parser_parse_primary(parser_t* parser)
{
  ast_t* main = NULL;
  switch (parser_peek(parser)->type) {
    case TOK_DOLLAR:
      parser_advance(parser);
      main = parser_parse_var_ref(parser);
      main->var_ref.is_global = true;
      break;
    case TOK_ID:
      main = parser_parse_id(parser);
      break;
    case TOK_LPAREN:
      parser_advance(parser); // '('
      main = parser_parse_expr(parser);
      parser_eat(parser, TOK_RPAREN);
      break;
    case TOK_LBRACK:
      main = parser_parse_list(parser);
      break;
    case TOK_LBRACE:
      main = parser_parse_map(parser);
      break;
    case TOK_INT:
      main = parser_parse_int(parser);
      break;
    case TOK_FLOAT:
      main = parser_parse_float(parser);
      break;
    case TOK_STRING:
      main = parser_parse_string(parser);
      break;
    case TOK_TRUE:
      parser_advance(parser);
      main = ast_true();
      break;
    case TOK_FALSE:
      parser_advance(parser);
      main = ast_false();
      break;
    case TOK_NULL:
      parser_advance(parser);
      main = ast_null();
      break;
    case TOK_IF:
      main = parser_parse_ternary(parser);
      break;
    default: {
      char err[ERR_LEN];
      sprintf(err, "invalid expression: \'%s\'", parser_peek(parser)->val);
      return parser_error(parser, err);
    }
  }

  // handle postfix operators
  while (true) {
    if (parser_match(parser, TOK_LBRACK)) { // subscript
      parser_advance(parser); // '['
      ast_t* index = parser_parse_expr(parser);
      parser_eat(parser, TOK_RBRACK); // require ']'

      ast_t* subscript = static_create_ast(AST_SUBSCRIPT, main->line);
      subscript->subscript.main = main;
      subscript->subscript.index = index;

      main = subscript; // assign to 'main'
    } else if (parser_match(parser, TOK_PERIOD)) { // member access
      parser_advance(parser); // '.'
      ast_t* mem = parser_parse_var_ref(parser);

      ast_t* memacc = static_create_ast(AST_MEMACC, main->line);
      memacc->memacc.main = main;
      memacc->memacc.mem = mem;

      main = memacc; // assign to 'main'
    } else if (parser_match(parser, TOK_DPERIOD)) { // method call
      parser_advance(parser); // '..'
      ast_t* mem = parser_parse_var_ref(parser);

      ast_t* memacc = static_create_ast(AST_MEMACC, main->line);
      memacc->memacc.main = main;
      memacc->memacc.mem = mem;
      main = memacc;

      ast_t* meth_call = parser_parse_func_call(parser);
      meth_call->func_call.is_method = true;
      meth_call->func_call.main = main;

      main = meth_call;
    } else if (parser_match(parser, TOK_LPAREN)) { // func call
      ast_t* func_call = parser_parse_func_call(parser); // must be only 'function call'

      func_call->func_call.is_method = false;
      func_call->func_call.main = main;

      main = func_call;
    } else {
      break;
    }
  }

  // handle assignment
  if (is_lvalue(main) &&
      (parser_match(parser, TOK_ASSIGN) ||
       parser_match(parser, TOK_PLUS_ASSIGN) ||
       parser_match(parser, TOK_MINUS_ASSIGN) ||
       parser_match(parser, TOK_MUL_ASSIGN) ||
       parser_match(parser, TOK_DIV_ASSIGN) ||
       parser_match(parser, TOK_MOD_ASSIGN) ||
       parser_match(parser, TOK_BAND_ASSIGN) ||
       parser_match(parser, TOK_BOR_ASSIGN) || 
       parser_match(parser, TOK_XOR_ASSIGN) || 
       parser_match(parser, TOK_SHL_ASSIGN) || 
       parser_match(parser, TOK_SHR_ASSIGN)
      )
     ) {
    ast_t* assign = static_create_ast(AST_ASSIGN, parser_line(parser));
    assign->assign.op_type = parser_advance(parser)->type; // assign type
    assign->assign.var = main;
    assign->assign.expr = parser_parse_expr(parser);
    main = assign;
  } else if (is_lvalue(main) && (parser_match(parser, TOK_INC) || parser_match(parser, TOK_DEC))) {
    ast_t* unary = static_create_ast(AST_UNARY, parser_line(parser));
    unary->unary.expr = main;
    unary->unary.op_type = parser_advance(parser)->type;
    unary->unary.type = POSTFIX;
    main = unary;
  }

  return main;
}
/* parse others */
static ast_t* parser_parse_include(parser_t* parser)
{
  parser_advance(parser); // 'include'
  ast_t* ast = static_create_ast(AST_INCLUDE, parser_line(parser));
  ast->include.symbols_size = 0;
  if (parser_match(parser, TOK_ID)) {
    ast->include.name = parser_eat(parser, TOK_ID)->val;

    if (parser_match(parser, TOK_AS)) {
      parser_advance(parser);
      ast->include.alias = parser_eat(parser, TOK_ID)->val;
    } else if (parser_match(parser, TOK_COLON)) {
      parser_advance(parser);

      ast->include.symbols = SEAL_CALLOC(1, sizeof(char*));
      ast->include.symbols[0] = parser_eat(parser, TOK_ID)->val;
      ast->include.symbols_size = 1;

      while (parser_match(parser, TOK_COMMA)) {
        parser_advance(parser); /* comma */

        ast->include.symbols = SEAL_REALLOC(ast->include.symbols, sizeof(char*) * ++ast->include.symbols_size);
        ast->include.symbols[ast->include.symbols_size - 1] = parser_eat(parser, TOK_ID)->val;
      }
    } else {
      ast->include.alias = NULL;
    }
  }
  
  return ast;
}
