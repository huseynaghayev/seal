#include "compiler.h"
#include "ast.h"
#include "hashmap.h"
#include "sealtypes.h"

#define START_BYTECODE_CAP 8
#define START_POOL_CAP     8

#define EMIT(bc, byte) do { \
    if ((bc)->size >= (bc)->cap) { \
      (bc)->bytecodes = SEAL_REALLOC((bc)->bytecodes, sizeof(seal_byte) * ((bc)->cap *= 2)); \
    } \
    (bc)->bytecodes[(bc)->size++] = (seal_byte)(byte); \
  } while (0)

#define EMIT_DUMMY(bc, times) for (int i = 0; i < (times); i++) { \
    EMIT(bc, 0); \
  }

#define REPLACE_16BITS_INDEX(addr, idx) do { \
    *(addr) = (seal_byte)((idx) >> 8); \
    *(addr + 1)   = (seal_byte)(idx); \
  } while (0)

#define SET_16BITS_INDEX(bc, idx) do { \
    EMIT(bc, (seal_word)(idx) >> 8); \
    EMIT(bc, (seal_word)(idx)); \
  } while (0)

#define CUR_IDX(bc) ((bc)->size) /* returns index of current empty byte */
#define LAST_OPCODE(bc) ((bc)->bytecodes[(bc)->size - 1]) /* returns last pushed opcode */
#define CUR_ADDR(bc) (&((bc)->bytecodes[CUR_IDX(bc)])) /* returns address of current empty byte */
#define CUR_ADDR_OFFSET(bc) (&((bc)->bytecodes[CUR_IDX(bc)]) - (bc)->bytecodes) /* returns address of current empty byte */

#define PUSH_CONST(pool, val) do { \
    /* check bounds */ \
    if ((pool)->size >= (pool)->cap) { \
      (pool)->vals = SEAL_REALLOC((pool)->vals, sizeof(svalue_t) * ((pool)->cap *= 2)); \
    } \
    (pool)->vals[(pool)->size++] = (val); \
} while (0)

#define CONST_IDX(pool) ((seal_word)((pool)->size - 1)) /* WARNING!! only use this after pushing constant */

#define PUSH_LABEL(pool, addr) do { \
    /* check bounds */ \
    if ((pool)->size >= (pool)->cap) { \
      (pool)->addrs = SEAL_REALLOC((pool)->addrs, sizeof(seal_word) * ((pool)->cap *= 2)); \
    } \
    (pool)->addrs[(pool)->size++] = (addr); \
} while (0)

#define LABEL_IDX(pool) ((seal_word)((pool)->size - 1)) /* WARNING!! only use this after pushing label */

#define __compiler_error(...) do { \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(1); \
  } while (0)

#define AUG_ASSIGN_OP_TYPE(type) ( \
  (type) == TOK_PLUS_ASSIGN ? OP_ADD : \
  (type) == TOK_MINUS_ASSIGN ? OP_SUB : \
  (type) == TOK_MUL_ASSIGN ? OP_MUL : \
  (type) == TOK_DIV_ASSIGN ? OP_DIV : \
  (type) == TOK_MOD_ASSIGN ? OP_MOD : \
  (type) == TOK_BAND_ASSIGN ? OP_AND : \
  (type) == TOK_BOR_ASSIGN ? OP_OR : \
  (type) == TOK_XOR_ASSIGN ? OP_XOR : \
  (type) == TOK_SHL_ASSIGN ? OP_SHL : \
  (type) == TOK_SHR_ASSIGN ? OP_SHR : \
  -1)


void compile(cout_t* cout, ast_t* node)
{
  struct scope main_scope = {
    .cp = {
      .cap = START_POOL_CAP,
      .size = 0,
      .vals = SEAL_CALLOC(START_POOL_CAP, sizeof(svalue_t)),
    },
    .bc = {
      .cap = START_BYTECODE_CAP,
      .size = 0,
      .bytecodes = SEAL_CALLOC(START_BYTECODE_CAP, sizeof(seal_byte))
    },
    .lp = {
      .cap = START_POOL_CAP,
      .size = 0,
      .addrs = SEAL_CALLOC(START_POOL_CAP, sizeof(seal_word))
    }
  };
  ///* constant values' pool */
  //cout->const_pool_ptr = cout->const_pool = SEAL_CALLOC(CONST_POOL_SIZE, sizeof(svalue_t));

  ///* label array */
  //cout->label_ptr = cout->labels = SEAL_CALLOC(LABEL_SIZE, sizeof(seal_word));

  //cout->bc.bytecodes = SEAL_CALLOC(START_BYTECODE_CAP, sizeof(seal_byte));

  ///* skip address offset stack */
  cout->skip_addr_offset_stack = SEAL_CALLOC(UNCOND_JMP_MAX_SIZE, sizeof(size_t));

  cout->stop_addr_offset_stack = SEAL_CALLOC(UNCOND_JMP_MAX_SIZE, sizeof(size_t));

  cout->skip_size = 0;
  cout->stop_size = 0;

  struct h_entry entries[LOCAL_MAX];
  hashmap_init_static(&main_scope.loctable, entries, LOCAL_MAX);

  compile_node(cout, node, &main_scope);

  cout->main_scope_local_size = main_scope.loctable.filled;
  cout->labels = main_scope.lp.addrs;
  cout->const_pool = main_scope.cp.vals;
  cout->const_pool_size = main_scope.cp.size;
  cout->label_pool_size = main_scope.lp.size;

  EMIT(&main_scope.bc, OP_HALT); /* push halt opcode for termination */
  cout->bc = main_scope.bc;
}
static void compile_node(cout_t* cout, ast_t* node, struct scope *s)
{
  switch (node->type) {
  case AST_COMP:
    for (int i = 0; i < node->comp.stmt_size; i++) {
      /*
       * DO NOT COMPILE DUMMY STATEMENTS (like 0, 1, 'a')
       * ONLY COMPILE FUNCTIONS AND POP THEIR VALUE 
       */
      switch (node->comp.stmts[i]->type) {
        case AST_NULL:
        case AST_INT:
        case AST_FLOAT:
        case AST_STRING:
        case AST_BOOL:
          break;
        case AST_FUNC_DEF:
          compile_node(cout, node->comp.stmts[i], s);
          if (node->comp.stmts[i]->func_def.name == NULL)
            EMIT(&s->bc, OP_POP);
          break;
        case AST_UNARY:
        case AST_BINARY:
        case AST_BINARY_BOOL:
        case AST_TERNARY:
        case AST_ASSIGN:
        case AST_VAR_REF:
        case AST_FUNC_CALL:
        case AST_LIST:
        case AST_MAP:
        case AST_SUBSCRIPT:
        case AST_MEMACC:
        case AST_INCLUDE:
          compile_node(cout, node->comp.stmts[i], s);
          EMIT(&s->bc, OP_POP);
          break;
        default:
          compile_node(cout, node->comp.stmts[i], s);
          break;
      }
    }
  break;
  case AST_NULL: case AST_INT: case AST_FLOAT: case AST_STRING: case AST_BOOL:
    compile_val(cout, node, s);
    break;
  case AST_NOP:
    break;
  case AST_IF: compile_if(cout, node, s); break;
  case AST_WHILE: compile_while(cout, node, s); break;
  case AST_DOWHILE: compile_dowhile(cout, node, s); break;
  case AST_FOR: compile_for(cout, node, s); break;
  case AST_SKIP: compile_skip(cout, s); break;
  case AST_STOP: compile_stop(cout, s); break;
  case AST_UNARY: compile_unary(cout, node, s); break;
  case AST_BINARY: compile_binary(cout, node, s); break;
  case AST_BINARY_BOOL: compile_logical_binary(cout, node, s); break;
  case AST_FUNC_CALL: compile_func_call(cout, node, s); break;
  case AST_ASSIGN: compile_assign(cout, node, s); break;
  case AST_VAR_REF: compile_var_ref(cout, node, s); break;
  case AST_FUNC_DEF: compile_func_def(cout, node, s); break;
  case AST_RETURN: compile_return(cout, node, s); break;
  case AST_LIST: compile_list(cout, node, s); break;
  case AST_MAP: compile_map(cout, node, s); break;
  case AST_SUBSCRIPT: compile_subscript(cout, node, s); break;
  case AST_MEMACC: compile_memacc(cout, node, s); break;
  case AST_INCLUDE: compile_include(cout, node, s); break;
  case AST_TERNARY: compile_ternary(cout, node, s); break;
  default:
    printf("%s is not implemented yet\n", hast_type_name(ast_type(node)));
    exit(1);
  }
}
static void compile_if(cout_t* cout, ast_t* node, struct scope *s)
{
  int jmp_size = 0;
  ast_t* temp_node = node;
  while (temp_node->type == AST_IF && temp_node->_if.has_else) {
    temp_node = temp_node->_if._else;
    jmp_size++;
  }
  size_t end_addr_offsets[jmp_size], *end_addr_offset = end_addr_offsets, next_addr_offset;

  compile_node(cout, node->_if.cond, s);

  EMIT(&s->bc, OP_JFALSE);
  next_addr_offset = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2); /* push 2 dummy values that will be changed later */

  compile_node(cout, node->_if.comp, s);

  if (jmp_size == 0) {
    PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
    REPLACE_16BITS_INDEX(s->bc.bytecodes + next_addr_offset, LABEL_IDX(&s->lp));
    return;
  }
  
  EMIT(&s->bc, OP_JUMP);
  *end_addr_offset++ = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2);

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  REPLACE_16BITS_INDEX(s->bc.bytecodes + next_addr_offset, LABEL_IDX(&s->lp));

  do {
    node = node->_if._else;

    if (node->type == AST_IF) {
      compile_node(cout, node->_if.cond, s);

      EMIT(&s->bc, OP_JFALSE);
      next_addr_offset = CUR_ADDR_OFFSET(&s->bc);
      EMIT_DUMMY(&s->bc, 2);

      compile_node(cout, node->_if.comp, s);

      if (end_addr_offset - end_addr_offsets != jmp_size) {
        if (node->_if.has_else) {
          EMIT(&s->bc, OP_JUMP);
          *end_addr_offset++ = CUR_ADDR_OFFSET(&s->bc);
          EMIT_DUMMY(&s->bc, 2);
        }
      }

      PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
      REPLACE_16BITS_INDEX(s->bc.bytecodes + next_addr_offset, LABEL_IDX(&s->lp));
    } else {
      compile_node(cout, node->_else.comp, s);
    }
  } while (node->_if.has_else);

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));

  for (int i = 0; i < jmp_size; i++) {
    REPLACE_16BITS_INDEX(s->bc.bytecodes + end_addr_offsets[i], LABEL_IDX(&s->lp));
  }
}
static void compile_while(cout_t* cout, ast_t* node, struct scope *s)
{
  size_t skip_start_size = cout->skip_size;
  size_t stop_start_size = cout->stop_size;

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  seal_word start = LABEL_IDX(&s->lp);
  compile_node(cout, node->_while.cond, s);

  EMIT(&s->bc, OP_JFALSE);
  size_t end_addr_offs = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2);

  compile_node(cout, node->_while.comp, s);

  EMIT(&s->bc, OP_JUMP);
  SET_16BITS_INDEX(&s->bc, start);
  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  REPLACE_16BITS_INDEX(s->bc.bytecodes + end_addr_offs, LABEL_IDX(&s->lp));

  if (skip_start_size < cout->skip_size) {
    for (int i = skip_start_size; i < cout->skip_size; i++) {
      REPLACE_16BITS_INDEX(s->bc.bytecodes + cout->skip_addr_offset_stack[i], start);
    }
  }
  cout->skip_size = skip_start_size;
  if (stop_start_size < cout->stop_size) {
    for (int i = stop_start_size; i < cout->stop_size; i++) {
      REPLACE_16BITS_INDEX(s->bc.bytecodes + cout->stop_addr_offset_stack[i], LABEL_IDX(&s->lp));
    }
  }
  cout->stop_size = stop_start_size;
}
static void compile_dowhile(cout_t* cout, ast_t* node, struct scope *s)
{
  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  seal_word start = LABEL_IDX(&s->lp);

  compile_node(cout, node->_while.comp, s);
  compile_node(cout, node->_while.cond, s);

  EMIT(&s->bc, OP_JTRUE);
  SET_16BITS_INDEX(&s->bc, start);
}
static void compile_for(cout_t *cout, ast_t *node, struct scope *s)
{
  size_t skip_start_size = cout->skip_size;
  size_t stop_start_size = cout->stop_size;

  compile_node(cout, node->_for.ited, s);
  EMIT(&s->bc, OP_PUSH_INT);
  SET_16BITS_INDEX(&s->bc, 1);  
  EMIT(&s->bc, OP_PUSH_INT);
  SET_16BITS_INDEX(&s->bc, 0);  

  const char *it_name = node->_for.it_name;
  struct h_entry *e = hashmap_search(&s->loctable, it_name);
  if (e == NULL)
    __compiler_error("maximum number of locals is %d", LOCAL_MAX);
  if (e->key == NULL)
    hashmap_insert_e(&s->loctable, e, it_name, SEAL_VALUE_INT((&s->loctable)->filled));

  EMIT(&s->bc, OP_FOR_PREP);
  size_t end_addr_offs = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2);

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  seal_word start_addr = LABEL_IDX(&s->lp);
  compile_node(cout, node->_for.comp, s);

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
  REPLACE_16BITS_INDEX(s->bc.bytecodes + end_addr_offs, LABEL_IDX(&s->lp));


  EMIT(&s->bc, OP_FOR_NEXT);
  EMIT(&s->bc, e->val.as._int); /* push slot index of local table */
  SET_16BITS_INDEX(&s->bc, start_addr);

  if (skip_start_size < cout->skip_size) {
    for (int i = skip_start_size; i < cout->skip_size; i++) {
      REPLACE_16BITS_INDEX(s->bc.bytecodes + cout->skip_addr_offset_stack[i], LABEL_IDX(&s->lp));
    }
  }
  cout->skip_size = skip_start_size;
  if (stop_start_size < cout->stop_size) {
    PUSH_LABEL(&s->lp, CUR_IDX(&s->bc));
    for (int i = stop_start_size; i < cout->stop_size; i++) {
      REPLACE_16BITS_INDEX(s->bc.bytecodes + cout->stop_addr_offset_stack[i], LABEL_IDX(&s->lp));
      if (LAST_OPCODE(&s->bc) != OP_FOR_STOP)
        EMIT(&s->bc, OP_FOR_STOP);
      /*
      EMIT(&s->bc, OP_POP);
      EMIT(&s->bc, OP_POP);
      EMIT(&s->bc, OP_POP);
      */
    }
  }
  cout->stop_size = stop_start_size;
}
static inline void compile_skip(cout_t* cout, struct scope *s)
{
  if (cout->skip_size >= UNCOND_JMP_MAX_SIZE)
    __compiler_error("maximum number of skip has exceeded");
  EMIT(&s->bc, OP_JUMP);
  cout->skip_addr_offset_stack[cout->skip_size++] = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2);
}
static inline void compile_stop(cout_t* cout, struct scope *s)
{
  if (cout->stop_size >= UNCOND_JMP_MAX_SIZE)
    __compiler_error("maximum number of stop has exceeded");
  EMIT(&s->bc, OP_JUMP);
  cout->stop_addr_offset_stack[cout->stop_size++] = CUR_ADDR_OFFSET(&s->bc);
  EMIT_DUMMY(&s->bc, 2);
}
static void compile_unary(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->unary.expr, s);

  seal_byte opcode;
  switch (node->unary.op_type) {
  case TOK_NOT   : opcode = OP_NOT; break;
  case TOK_MINUS : opcode = OP_NEG; break;
  case TOK_TYPEOF: opcode = OP_TYPOF; break;
  case TOK_BNOT  : opcode = OP_BNOT; break;
  case TOK_PLUS  : return;
  }

  EMIT(&s->bc, opcode);
}
static void compile_binary(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->binary.left, s);
  compile_node(cout, node->binary.right, s);
  
  seal_byte opcode;
  switch (node->binary.op_type) {
  case TOK_PLUS : opcode = OP_ADD; break;
  case TOK_MINUS: opcode = OP_SUB; break;
  case TOK_MUL  : opcode = OP_MUL; break;
  case TOK_DIV  : opcode = OP_DIV; break;
  case TOK_MOD  : opcode = OP_MOD; break;
  case TOK_BAND : opcode = OP_AND; break;
  case TOK_BOR  : opcode = OP_OR;  break;
  case TOK_XOR  : opcode = OP_XOR; break;
  case TOK_SHL  : opcode = OP_SHL; break;
  case TOK_SHR  : opcode = OP_SHR; break;
  case TOK_EQ   : opcode = OP_EQ;  break;
  case TOK_NE   : opcode = OP_NE;  break;
  case TOK_GT   : opcode = OP_GT;  break;
  case TOK_GE   : opcode = OP_GE;  break;
  case TOK_LT   : opcode = OP_LT;  break;
  case TOK_LE   : opcode = OP_LE;  break;
  case TOK_IN   : opcode = OP_IN;  break;
  }

  EMIT(&s->bc, opcode);
}
static void compile_logical_binary(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->binary.left, s);

  EMIT(&s->bc, OP_DUP); /* duplicate to compare for jumping */

  if (node->binary.op_type == TOK_AND) {
    EMIT(&s->bc, OP_JFALSE);
  } else {
    EMIT(&s->bc, OP_JTRUE);
  }
  size_t end_addr_offset = CUR_ADDR_OFFSET(&s->bc); /* store end address offset */
  EMIT_DUMMY(&s->bc, 2); /* push zero bytes */
  EMIT(&s->bc, OP_POP); /* pop first value if no jump */

  compile_node(cout, node->binary.right, s); /* compile right side */

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc)); /* push end label */
  REPLACE_16BITS_INDEX(s->bc.bytecodes + end_addr_offset, LABEL_IDX(&s->lp)); /* replace zero bytes with end label index */
}
static void compile_val(cout_t* cout, ast_t* node, struct scope *s)
{
  svalue_t val;
  switch (node->type) {
  case AST_BOOL:
    EMIT(&s->bc, node->boolean.val ? OP_PUSH_TRUE : OP_PUSH_FALSE);
    return;
  case AST_NULL:
    EMIT(&s->bc, OP_PUSH_NULL);
    return;
  case AST_INT:
    if (node->integer.val <= 0xFFFF) {
      EMIT(&s->bc, OP_PUSH_INT);
      SET_16BITS_INDEX(&s->bc, node->integer.val);
      return;
    }
    val = sval(SEAL_INT, _int, node->integer.val);
    break;
  case AST_FLOAT:
    val = sval(SEAL_FLOAT, _float, node->floating.val);
    break;
  case AST_STRING:
    val = SEAL_VALUE_STRING_STATIC(node->string.val);
    break;
  }

  EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
  PUSH_CONST(&s->cp, val); /* push constant into pool */
  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
}
static void compile_func_call(cout_t* cout, ast_t* node, struct scope *s)
{
  if (!node->func_call.is_method) {
    if (node->func_call.arg_size > 255)
      __compiler_error("maximum number of arguments in a function call is 255");

    compile_node(cout, node->func_call.main, s);

    for (int i = 0; i < node->func_call.arg_size; i++)
      compile_node(cout, node->func_call.args[i], s);

    EMIT(&s->bc, OP_CALL);
    EMIT(&s->bc, node->func_call.arg_size);
    return;
  }
  if (node->func_call.arg_size > 254)
    __compiler_error("maximum number of arguments in a method call is 254");

  compile_node(cout, node->func_call.main->memacc.main, s);
  EMIT(&s->bc, OP_DUP);
  EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
  PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->func_call.main->memacc.mem->var_ref.name)); /* push constant into pool */
  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
  EMIT(&s->bc, OP_GET_FIELD);
  EMIT(&s->bc, OP_SWAP);
  EMIT(&s->bc, 2);

  for (int i = 0; i < node->func_call.arg_size; i++)
    compile_node(cout, node->func_call.args[i], s);

  EMIT(&s->bc, OP_CALL);
  EMIT(&s->bc, node->func_call.arg_size + 1);
}
static void compile_assign(cout_t* cout, ast_t* node, struct scope *s)
{
  int lval_type = node->assign.var->type;
  svalue_t sym;
  const char* name;
  struct h_entry* e;

  if (node->assign.op_type == TOK_ASSIGN) {
    compile_node(cout, node->assign.expr, s);

    switch (lval_type) {
    case AST_VAR_REF:
      if (node->assign.var->var_ref.is_global) {
        EMIT(&s->bc, OP_SET_GLOBAL);
        sym = SEAL_VALUE_STRING_STATIC(node->assign.var->var_ref.name);
        PUSH_CONST(&s->cp, sym);
        SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
      } else {
        EMIT(&s->bc, OP_SET_LOCAL);
        name = node->assign.var->var_ref.name;
        e = hashmap_search(&s->loctable, name);
        if (e == NULL)
          __compiler_error("maximum number of locals is %d", LOCAL_MAX);

        if (e->key == NULL)
          hashmap_insert_e(&s->loctable, e, name, SEAL_VALUE_INT((&s->loctable)->filled));

        EMIT(&s->bc, e->val.as._int); /* push slot index of local table */
      }
      break;
    case AST_SUBSCRIPT:
      compile_node(cout, node->assign.var->subscript.main, s);
      compile_node(cout, node->assign.var->subscript.index, s);
      EMIT(&s->bc, OP_SET_FIELD);
      break;
    case AST_MEMACC:
      compile_node(cout, node->assign.var->memacc.main, s);
      EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
      PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->assign.var->memacc.mem->var_ref.name)); /* push constant into pool */
      SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
      EMIT(&s->bc, OP_SET_FIELD);
      break;
    default:
      __compiler_error("assigning to %s is not implemented yet", hast_type_name(lval_type));
      break;
    }
  } else {
    seal_word sym_idx;
    int aug_type = node->assign.op_type;
    switch (lval_type) {
    case AST_VAR_REF:
      if (node->assign.var->var_ref.is_global) {
        EMIT(&s->bc, OP_GET_GLOBAL);
        sym = SEAL_VALUE_STRING_STATIC(node->assign.var->var_ref.name);
        PUSH_CONST(&s->cp, sym);
        sym_idx = CONST_IDX(&s->cp);
        SET_16BITS_INDEX(&s->bc, sym_idx);
        compile_node(cout, node->assign.expr, s);
        EMIT(&s->bc, AUG_ASSIGN_OP_TYPE(aug_type));
        EMIT(&s->bc, OP_SET_GLOBAL);
        SET_16BITS_INDEX(&s->bc, sym_idx);
      } else {
        EMIT(&s->bc, OP_GET_LOCAL);
        name = node->assign.var->var_ref.name;
        e = hashmap_search(&s->loctable, name);
        if (e == NULL || e->key == NULL)
          __compiler_error("\'%s\' is not defined", name);

        EMIT(&s->bc, e->val.as._int); /* push slot index of local table */
        compile_node(cout, node->assign.expr, s);
        EMIT(&s->bc, AUG_ASSIGN_OP_TYPE(aug_type));
        EMIT(&s->bc, OP_SET_LOCAL);
        EMIT(&s->bc, e->val.as._int);
      }
      break;
    case AST_SUBSCRIPT:
      compile_node(cout, node->assign.var->subscript.main, s);
      compile_node(cout, node->assign.var->subscript.index, s);
      EMIT(&s->bc, OP_COPY);
      EMIT(&s->bc, 2);
      EMIT(&s->bc, OP_COPY);
      EMIT(&s->bc, 2);
      EMIT(&s->bc, OP_GET_FIELD);
      compile_node(cout, node->assign.expr, s);
      EMIT(&s->bc, AUG_ASSIGN_OP_TYPE(aug_type));
      EMIT(&s->bc, OP_SWAP);
      EMIT(&s->bc, 3);
      EMIT(&s->bc, OP_SWAP);
      EMIT(&s->bc, 2);
      EMIT(&s->bc, OP_SET_FIELD);
      break;
    case AST_MEMACC:
      compile_node(cout, node->assign.var->memacc.main, s);
      EMIT(&s->bc, OP_DUP);
      EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
      PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->assign.var->memacc.mem->var_ref.name)); /* push constant into pool */
      sym_idx = CONST_IDX(&s->cp);
      SET_16BITS_INDEX(&s->bc, sym_idx);

      EMIT(&s->bc, OP_GET_FIELD);

      compile_node(cout, node->assign.expr, s);

      EMIT(&s->bc, AUG_ASSIGN_OP_TYPE(aug_type));

      EMIT(&s->bc, OP_SWAP);
      EMIT(&s->bc, 2);

      EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
      SET_16BITS_INDEX(&s->bc, sym_idx);
      EMIT(&s->bc, OP_SET_FIELD);
      break;
    default:
      __compiler_error("assigning to %s is not implemented yet", hast_type_name(lval_type));
      break;
    }
  }
}
static void compile_var_ref(cout_t* cout, ast_t* node, struct scope *s)
{
  if (!node->var_ref.is_global) {
    struct h_entry* e = hashmap_search(&s->loctable, node->var_ref.name);
    if (e == NULL || e->key == NULL)
      goto global;

    EMIT(&s->bc, OP_GET_LOCAL);
    EMIT(&s->bc, e->val.as._int);
  } else {
global:
    EMIT(&s->bc, OP_GET_GLOBAL);
    PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->var_ref.name));
    SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
  }
}
static void compile_func_def(cout_t* cout, ast_t* node, struct scope *s)
{
  struct scope loc_scope = {
    .cp = {
      .cap = START_POOL_CAP,
      .size = 0,
      .vals = SEAL_CALLOC(START_POOL_CAP, sizeof(svalue_t)),
    },
    .bc = {
      .cap = START_BYTECODE_CAP,
      .size = 0,
      .bytecodes = SEAL_CALLOC(START_BYTECODE_CAP, sizeof(seal_byte))
    },
    .lp = {
      .cap = START_POOL_CAP,
      .size = 0,
      .addrs = SEAL_CALLOC(START_POOL_CAP, sizeof(seal_word))
    }
  };
  struct h_entry entries[LOCAL_MAX];
  hashmap_init_static(&loc_scope.loctable, entries, LOCAL_MAX);
  for (int i = 0; i < node->func_def.param_size; i++) {
    hashmap_insert(&loc_scope.loctable, node->func_def.param_names[i], SEAL_VALUE_INT(i));
  }

  svalue_t func_obj = {
    .type = SEAL_FUNC,
    .as.func = {
      .type = FUNC_USERDEF,
      .is_vararg = false,
      .name = node->func_def.name,
      .as.userdef = {
        .argc = node->func_def.param_size,
        .globals = NULL
      }
    }
  };

  compile_node(cout, node->func_def.comp, &loc_scope);
  EMIT(&loc_scope.bc, OP_PUSH_NULL);
  EMIT(&loc_scope.bc, OP_HALT);
  func_obj.as.func.as.userdef.bytecode = loc_scope.bc.bytecodes;
  func_obj.as.func.as.userdef.const_pool = loc_scope.cp.vals;
  func_obj.as.func.as.userdef.label_pool = loc_scope.lp.addrs;
  func_obj.as.func.as.userdef.local_size = loc_scope.loctable.filled; /* assign size of locals */

  EMIT(&s->bc, OP_PUSH_CONST); /* push function object to constant pool */
  PUSH_CONST(&s->cp, func_obj);
  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));

  bool is_anonym = node->func_def.name == NULL;

  if (!is_anonym) {
    EMIT(&s->bc, OP_SET_GLOBAL); /* set function object to global */
    PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->func_def.name));
    SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
    EMIT(&s->bc, OP_POP);
  }
}
static void compile_return(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->_return.expr, s);
  EMIT(&s->bc, OP_HALT);
}
static void compile_list(cout_t* cout, ast_t* node, struct scope *s)
{
  if (node->list.mem_size > 255)
    __compiler_error("maximum number of elements in a list initializer is 255");
  for (int i = 0; i < node->list.mem_size; i++)
    compile_node(cout, node->list.mems[i], s);

  EMIT(&s->bc, OP_GEN_LIST);
  EMIT(&s->bc, node->list.mem_size);
}
static void compile_subscript(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->subscript.main, s);
  compile_node(cout, node->subscript.index, s);
  EMIT(&s->bc, OP_GET_FIELD);
}
static void compile_map(cout_t* cout, ast_t* node, struct scope *s)
{
  if (node->map.field_size > 255)
    __compiler_error("maximum number of elements in a map initializer is 255");
  for (int i = 0; i < node->map.field_size; i++) {
    compile_node(cout, node->map.field_vals[i], s);
    EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
    PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->map.field_names[i])); /* push constant into pool */
    SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
  }

  EMIT(&s->bc, OP_GEN_MAP);
  EMIT(&s->bc, node->map.field_size);
}
static void compile_memacc(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->memacc.main, s);
  EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
  PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->memacc.mem->var_ref.name)); /* push constant into pool */
  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
  EMIT(&s->bc, OP_GET_FIELD);
}
static void compile_include(cout_t *cout, ast_t *node, struct scope *s)
{
  EMIT(&s->bc, OP_PUSH_CONST); /* push opcode */
  PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->include.name)); /* push module name into pool */
  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));

  EMIT(&s->bc, OP_INCLUDE);
  EMIT(&s->bc, OP_SET_GLOBAL);
  if (node->include.alias)
    PUSH_CONST(&s->cp, SEAL_VALUE_STRING_STATIC(node->include.alias)); /* push alias name into pool */

  SET_16BITS_INDEX(&s->bc, CONST_IDX(&s->cp));
}
static void compile_ternary(cout_t* cout, ast_t* node, struct scope *s)
{
  compile_node(cout, node->ternary.cond, s); /* compile condition */

  EMIT(&s->bc, OP_JFALSE); /* if false, jump to else (false) expression */
  size_t else_addr_offset = CUR_ADDR_OFFSET(&s->bc); /* store else address offset */
  EMIT_DUMMY(&s->bc, 2); /* push zero bytes */

  compile_node(cout, node->ternary.expr_true, s);

  EMIT(&s->bc, OP_JUMP);
  size_t end_addr_offset = CUR_ADDR_OFFSET(&s->bc); /* store end address offset */
  EMIT_DUMMY(&s->bc, 2); /* push zero bytes */

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc)); /* push else label */
  REPLACE_16BITS_INDEX(s->bc.bytecodes + else_addr_offset, LABEL_IDX(&s->lp)); /* replace zero bytes with else label index */

  compile_node(cout, node->ternary.expr_false, s);

  PUSH_LABEL(&s->lp, CUR_IDX(&s->bc)); /* push end label */
  REPLACE_16BITS_INDEX(s->bc.bytecodes + end_addr_offset, LABEL_IDX(&s->lp)); /* replace zero bytes with end label index */
}
