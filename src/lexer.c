#include "lexer.h"
#include "io.h"

#define LEXEME_ID                   0
#define LEXEME_DIGIT                1
#define LEXEME_FRACTION_BEGIN_DIGIT 2
#define LEXEME_STRING               3

#define SINGLE_LINE_COMMENT 0
#define MULTILINE_COMMENT   1

#define lexer_is_end(lexer) ( \
  lexer->i >= lexer->src_size)

#define lexer_peek_offset(lexer, offset) ( \
  (lexer->i + offset >= 0 && lexer->i + offset < lexer->src_size) ? lexer->src[lexer->i + offset] : '\0')

#define lexer_peek(lexer) ( \
  lexer_is_end(lexer) ? '\0' : lexer->src[lexer->i])
  
#define lexer_is_alpha(c) ( \
  (c >= 'a' && c <= 'z') || \
  (c >= 'A' && c <= 'Z'))

#define lexer_is_digit(c) ( \
  c >= '0' && c <= '9')

#define lexer_is_id(c) ( \
  lexer_is_alpha(c) || c == '_')

#define lexer_is_alnum_(c) ( \
  lexer_is_id(c) || lexer_is_digit(c))

static inline void lexer_error(lexer_t* lexer, const char* err, int line)
{
  fprintf(stderr, "seal: file: \'%s\', line %d\nsyntax error: %s\n", lexer->file_path, line == 0 ? lexer->line : line, err);
  exit(EXIT_FAILURE);
}

/* main functions */

inline void init_lexer(lexer_t* lexer, const char* file_path)
{
  const char* src = read_file(file_path);
  if (!src) {
    fprintf(stderr, "seal: cannot open \'%s\': No such file or directory\n", file_path);
    exit(EXIT_FAILURE);
  }
  lexer->file_path           = file_path;
  lexer->src                 = src;
  lexer->src_size            = strlen(src);
  lexer->i                   = 0;
  lexer->line                = 1;
  lexer->toks                = NULL;
  lexer->tok_size            = 0;
  lexer->cur_indent          = 0;
  lexer->encountered_word    = false;
  lexer->token_after_comment = false;

  lexer->indent_stack_ptr = lexer->paren_stack_ptr = lexer->paren_lines_ptr = 0; // init to 0

  memset(lexer->indent_stack, -1, sizeof(lexer->indent_stack)); // unitialized
  lexer->indent_stack[lexer->indent_stack_ptr] = 0; // global indentation block

  memset(lexer->paren_stack, -1, sizeof(lexer->paren_stack)); // no nest
  memset(lexer->paren_lines_stack, -1, sizeof(lexer->paren_lines_stack)); // no nest
}

inline void lexer_get_tokens(lexer_t* lexer)
{
  while (!lexer_is_end(lexer)) {
    lexer_get_token(lexer);
  }
  if (lexer->paren_stack_ptr > 0) {
    char err[ERR_LEN];
    sprintf(err, "'%c' was never closed", lexer->paren_stack[lexer->paren_stack_ptr]);
    lexer_error(lexer, err, lexer->paren_lines_stack[lexer->paren_lines_ptr]);
  }

  if (lexer->tok_size > 0 && lexer->toks[lexer->tok_size - 1]->type != TOK_NEWL) {
    lexer_add_token(lexer, create_token(TOK_NEWL, NULL, lexer->line));
  }

  for (int i = 0; i < lexer->indent_stack_ptr; i++) {
    lexer_add_token(lexer, create_token(TOK_DEDENT, NULL, lexer->line));
    lexer_add_token(lexer, create_token(TOK_NEWL, NULL, lexer->line));
  }

  lexer_add_token(lexer, create_token(TOK_EOF, NULL, lexer->line));

  SEAL_FREE((char*)lexer->src); // free source file after lexing
}

static inline void stack_push(int val, int stack[], int stack_size, int* stack_ptr)
{
  if (*stack_ptr == stack_size - 1) {
    fprintf(stderr, "%d stack limit exceeded\n", stack_size);
    exit(EXIT_FAILURE);
  }
  stack[++*stack_ptr] = val;
}

static inline int stack_pop(int stack[], int stack_size, int* stack_ptr)
{
  if (*stack_ptr == 0) {
    fprintf(stderr, "popping empty stack\n");
    exit(EXIT_FAILURE);
  }
  int popped = stack[*stack_ptr];
  stack[(*stack_ptr)--] = -1;
  return popped;
}

static void lexer_get_token(lexer_t* lexer)
{
  bool encountered_word = true;

  char c = lexer_advance(lexer);
  token_t* tok = NULL;

  switch (c) {
    case ' ': case '\t':
      if (!lexer->encountered_word) {
        lexer->cur_indent++;
        encountered_word = false;
      }
      break;
    case '\n': case '\r':
      lexer->cur_indent = 0;
      encountered_word  = false;
      if (lexer->encountered_word && lexer->paren_stack_ptr == 0) {
        tok = create_token(TOK_NEWL, NULL, lexer->line);
      }
      lexer->token_after_comment = false;
      lexer->line++;
      break;
    case '$':
      tok = create_token(TOK_DOLLAR, NULL, lexer->line);
      break;
    case '.':
      if (lexer_is_digit(lexer_peek(lexer)))
        tok = lexer_get_digit(lexer, LEXEME_FRACTION_BEGIN_DIGIT);
      else if (lexer_match(lexer, '.'))
        tok = create_token(TOK_DPERIOD, NULL, lexer->line);
      else
        tok = create_token(TOK_PERIOD, NULL, lexer->line);
      break;
    case ':':
      tok = create_token(TOK_COLON, NULL, lexer->line);
      break;
    case ',':
      tok = create_token(TOK_COMMA, NULL, lexer->line);
      break;
    case '+':
      if (lexer_match(lexer, '+'))
        tok = create_token(TOK_INC, NULL, lexer->line);
      else if (lexer_match(lexer, '='))
        tok = create_token(TOK_PLUS_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_PLUS, NULL, lexer->line);
      break;
    case '-':
      if (lexer_match(lexer, '-'))
        tok = create_token(TOK_DEC, NULL, lexer->line);
      else if (lexer_match(lexer, '='))
        tok = create_token(TOK_MINUS_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_MINUS, NULL, lexer->line);
      break;
    case '*':
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_MUL_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_MUL, NULL, lexer->line);
      break;
    case '/':
      if (lexer_match(lexer, '/')) {
        lexer_ignore_comment(lexer, SINGLE_LINE_COMMENT);
        encountered_word = lexer->encountered_word;
      } else if (lexer_match(lexer, '*')) {
        lexer_ignore_comment(lexer, MULTILINE_COMMENT);
        encountered_word = lexer->encountered_word;
      } else if (lexer_match(lexer, '='))
        tok = create_token(TOK_DIV_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_DIV, NULL, lexer->line);
      break;
    case '%':
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_MOD_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_MOD, NULL, lexer->line);
      break;
    case '&':
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_BAND_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_BAND, NULL, lexer->line);
      break;
    case '|':
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_BOR_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_BOR, NULL, lexer->line);
      break;
    case '^':
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_XOR_ASSIGN, NULL, lexer->line);
      else
        tok = create_token(TOK_XOR, NULL, lexer->line);
      break;
    case '~':
      tok = create_token(TOK_BNOT, NULL, lexer->line);
      break;
    case '=': // ==
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_EQ, NULL, lexer->line);
      else
        tok = create_token(TOK_ASSIGN, NULL, lexer->line);
      break;
    case '!': // !=
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_NE, NULL, lexer->line);
      else
        tok = create_token(TOK_NOT, "!", lexer->line);
      break;
    case '>': // >=
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_GE, NULL, lexer->line);
      else if (lexer_match(lexer, '>'))
        if (lexer_match(lexer, '='))
          tok = create_token(TOK_SHR_ASSIGN, NULL, lexer->line);
        else
          tok = create_token(TOK_SHR, NULL, lexer->line);
      else
        tok = create_token(TOK_GT, NULL, lexer->line);
      break;
    case '<': // <=
      if (lexer_match(lexer, '='))
        tok = create_token(TOK_LE, NULL, lexer->line);
      else if (lexer_match(lexer, '<'))
        if (lexer_match(lexer, '='))
          tok = create_token(TOK_SHL_ASSIGN, NULL, lexer->line);
        else
          tok = create_token(TOK_SHL, NULL, lexer->line);
      else
        tok = create_token(TOK_LT, NULL, lexer->line);
      break;
    case '(':
      tok = create_token(TOK_LPAREN, NULL, lexer->line);
      break;
    case ')':
      tok = create_token(TOK_RPAREN, NULL, lexer->line);
      break;
    case '[':
      tok = create_token(TOK_LBRACK, NULL, lexer->line);
      break;
    case ']':
      tok = create_token(TOK_RBRACK, NULL, lexer->line);
      break;
    case '{':
      tok = create_token(TOK_LBRACE, NULL, lexer->line);
      break;
    case '}':
      tok = create_token(TOK_RBRACE, NULL, lexer->line);
      break;
    case '\'': case '\"':
      tok = lexer_get_string(lexer, c);
      break;
    default:
      if (lexer_is_id(c))
        tok = lexer_get_id(lexer);
      else if (lexer_is_digit(c))
        tok = lexer_get_digit(lexer, LEXEME_DIGIT);
      else {
        char err[ERR_LEN];
        sprintf(err, "unexpected character: '%c'", c);
        lexer_error(lexer, err, 0);
      }
      break;
  }

  // indentation stack
  if (lexer->paren_stack_ptr == 0 && !lexer->encountered_word && encountered_word) {
    lexer->encountered_word = encountered_word;
    if (lexer->cur_indent > lexer->indent_stack[lexer->indent_stack_ptr]) {
      stack_push(lexer->cur_indent, lexer->indent_stack, MAX_INDENT_LEVEL, &lexer->indent_stack_ptr);
      lexer_add_token(lexer, create_token(TOK_INDENT, NULL, lexer->line));
    } else if (lexer->cur_indent < lexer->indent_stack[lexer->indent_stack_ptr]) {
      int times_pop = 0;
      pop_indent:
        stack_pop(lexer->indent_stack, MAX_INDENT_LEVEL, &lexer->indent_stack_ptr);
        times_pop++;
        if (lexer->cur_indent != lexer->indent_stack[lexer->indent_stack_ptr]) {
          if (lexer->indent_stack_ptr > 0) {
            goto pop_indent;
          } else {
            lexer_error(lexer, "mismatch unindetation", 0);
          }
        }
      /* end label pop_indent */

      for (int i = 0; i < times_pop; i++) {
        lexer_add_token(lexer, create_token(TOK_DEDENT, NULL, lexer->line));
        lexer_add_token(lexer, create_token(TOK_NEWL, NULL, lexer->line));
      }
    }
  }

  // parenthesis stack
  if (tok && tok->type >= TOK_LPAREN && tok->type <= TOK_RBRACE) {
    if (tok->type == TOK_LPAREN || tok->type == TOK_LBRACK || tok->type == TOK_LBRACE) {
      stack_push(c, lexer->paren_stack, MAX_NESTED_PAREN_LEVEL, &lexer->paren_stack_ptr);
      stack_push(lexer->line, lexer->paren_lines_stack, MAX_NESTED_PAREN_LEVEL, &lexer->paren_lines_ptr);
    } else {
      if (lexer->paren_stack_ptr == 0) {
        char err[ERR_LEN];
        sprintf(err, "unmatched '%c'", c);
        lexer_error(lexer, err, 0);
      }
      int popped = stack_pop(lexer->paren_stack, MAX_NESTED_PAREN_LEVEL, &lexer->paren_stack_ptr);
      int popped_line = stack_pop(lexer->paren_lines_stack, MAX_NESTED_PAREN_LEVEL, &lexer->paren_lines_ptr);
      switch (c) {
        case ')':
          if (popped != '(') goto error;
          break;
        case ']':
          if (popped != '[') goto error;
          break;
        case '}':
          if (popped != '{') goto error;
          break;
        error: {
          char err[ERR_LEN];
          sprintf(err, "'%c' does not balance '%c'", c, popped);
          lexer_error(lexer, err, 0);
        }
      }
    }
  }

  if (tok) {
    if (lexer->token_after_comment) {
      char err[ERR_LEN];
      sprintf(err, "token after comment block not allowed");
      lexer_error(lexer, err, 0);
    }
    lexer_add_token(lexer, tok);
  }

  lexer->encountered_word = encountered_word;
}

/* char functions */
static inline char lexer_advance(lexer_t* lexer)
{
  return lexer->src[lexer->i++];
}

static inline char lexer_match(lexer_t* lexer, char c)
{
  return lexer_peek(lexer) == c ? lexer_advance(lexer) : '\0';
}

/* token functions */
static inline void lexer_add_token(lexer_t* lexer, token_t* tok)
{
  lexer->tok_size++;
  lexer->toks = SEAL_REALLOC(lexer->toks, lexer->tok_size * sizeof(token_t*));
  lexer->toks[lexer->tok_size - 1] = tok;
}

static token_t* lexer_get_id(lexer_t* lexer)
{
  char* lexeme = lexer_get_lexeme(lexer, LEXEME_ID, 0);
  for (int i = TOK_IF; i < TOK_ID; i++) {
    if (strcmp(lexeme, htoken_type_name(i)) == 0) {
      free(lexeme);
      return create_token(i, NULL, lexer->line);
    }
  }
  return create_token(TOK_ID, lexeme, lexer->line);
}

static token_t* lexer_get_digit(lexer_t* lexer, int lexeme_type)
{
  token_t* tok = NULL;
  const char* lexeme = lexer_get_lexeme(lexer, lexeme_type, 0);
  if (lexeme_type == LEXEME_FRACTION_BEGIN_DIGIT) {
    tok = create_token(TOK_FLOAT, lexeme, lexer->line);
  } else if (lexeme_type == LEXEME_DIGIT) {
    for (const char* cp_lexeme = lexeme; *cp_lexeme != '\0'; cp_lexeme++) {
      if (*cp_lexeme == '.') {
        return create_token(TOK_FLOAT, lexeme, lexer->line);
      }
    }
    tok = create_token(TOK_INT, lexeme, lexer->line);
  } else {
    fprintf(stderr, "unrecognized digit type to get\n");
  }

  return tok;
}

static token_t* lexer_get_string(lexer_t* lexer, int str_sur)
{
  const char* lexeme = lexer_get_lexeme(lexer, LEXEME_STRING, str_sur);
  return create_token(TOK_STRING, lexeme ? lexeme : "", lexer->line);
}

/* custom append function for string */
static inline void lexeme_append(char** lexeme, size_t* l_size, char c)
{
  (*l_size)++;
  *lexeme = SEAL_REALLOC(*lexeme, (*l_size + 1) * sizeof(char));
  (*lexeme)[*l_size - 1] = c;
  (*lexeme)[*l_size] = '\0';
}

/* get lexeme */
static char* lexer_get_lexeme(lexer_t* lexer, int lexeme_type, int str_sur)
{
  char* lexeme  = NULL;
  size_t l_size = 0;

  if (lexeme_type != LEXEME_STRING) {
    lexeme_append(&lexeme, &l_size, lexer_peek_offset(lexer, -1));
  }

  char c;
  switch (lexeme_type) {
    case LEXEME_ID:
      while (lexer_is_alnum_((c = lexer_peek(lexer)))) {
        lexer_advance(lexer);
        lexeme_append(&lexeme, &l_size, c);
      }
      break;
    case LEXEME_DIGIT:
      while (lexer_is_digit((c = lexer_peek(lexer)))) {
        lexer_advance(lexer);
        lexeme_append(&lexeme, &l_size, c);
      }
      if (lexer_peek(lexer) == '.') {
        lexeme_append(&lexeme, &l_size, lexer_advance(lexer));
      }
      while (lexer_is_digit((c = lexer_peek(lexer)))) {
        lexer_advance(lexer);
        lexeme_append(&lexeme, &l_size, c);
      }
      break;
    case LEXEME_FRACTION_BEGIN_DIGIT:
      while (lexer_is_digit((c = lexer_peek(lexer)))) {
        lexer_advance(lexer);
        lexeme_append(&lexeme, &l_size, c);
      }
      break;
    case LEXEME_STRING:
      while ((c = lexer_peek(lexer)) != '\n' && c != str_sur) {
        lexer_advance(lexer);
        if (c == '\\') {
          char esc_c = lexer_advance(lexer);
          switch (esc_c) {
            case '\\':
              break;
            case 'n':
              c = '\n';
              break;
            case 'r':
              c = '\r';
              break;
            case 't':
              c = '\t';
              break;
            case '\'':
              c = '\'';
              break;
            case '\"':
              c = '\"';
              break;
            case 'b':
              c = '\b';
              break;
            default: {
              char err[ERR_LEN];
              sprintf(err, "invalid escape sequence '\\%c'", esc_c);
              lexer_error(lexer, err, 0);
            }
          }
        }
        lexeme_append(&lexeme, &l_size, c);
      }
      if (c != str_sur) {
        lexer_error(lexer, "unterminated string", 0);
      }
      lexer_advance(lexer);
      break;
  }

  return lexeme;
}

static void lexer_ignore_comment(lexer_t* lexer, int comment_type)
{
  if (comment_type == SINGLE_LINE_COMMENT) {
    while (!lexer_is_end(lexer) && lexer_peek(lexer) != '\n')
      lexer_advance(lexer);
  } else if (comment_type == MULTILINE_COMMENT) {
    int start_line = lexer->line;
    while (!lexer_is_end(lexer) && (lexer_peek(lexer) != '*' || lexer_peek_offset(lexer, 1) != '/'))
      if (lexer_advance(lexer) == '\n') lexer->line++;

    if (!lexer_match(lexer, '*') || !lexer_match(lexer, '/')) {
      char err[ERR_LEN];
      sprintf(err, "unclosed comment block");
      lexer_error(lexer, err, start_line);
    }

    if ((lexer->line != start_line || !lexer->encountered_word) && lexer->paren_stack_ptr == 0)
      lexer->token_after_comment = true;
  }
}
