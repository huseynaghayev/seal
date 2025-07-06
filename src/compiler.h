#ifndef SEAL_COMPILER_H
#define SEAL_COMPILER_H

#include "sealconf.h"
#include "ast.h"
#include "sealtypes.h"
#include "bytecode.h"
#include "hashmap.h"

typedef struct cout cout_t;

#define CONST_POOL_SIZE (0xFFFF + 1)    /* 65536 */
#define LABEL_SIZE      (0xFFFF + 1)    /* 65536 */
#define UNCOND_JMP_MAX_SIZE 1024

#define PRINT_CONST_POOL(cout) for (int i = 0; i < cout.const_pool_size; i++) { \
    if (i == 0) printf("CONST POOL START-------\n"); \
    svalue_t s = cout.const_pool[i]; \
    switch (s.type) { \
      case SEAL_INT: \
        printf("%lld\n", s.as._int); \
        break; \
      case SEAL_FLOAT: \
        printf("%f\n", s.as._float); \
        break; \
      case SEAL_STRING: \
        printf("\'%s\'\n", s.as.string->val); \
        break; \
      case SEAL_BOOL: \
        printf("%s\n", s.as._bool ? "true" : "false"); \
        break; \
      case SEAL_NULL: \
        printf("null\n"); \
        break; \
      case SEAL_FUNC: \
        printf("\'%s\' function\n", s.as.func.name); \
        break; \
      default: \
        printf("UNRECOGNIZED DATA TYPE TO PRINT\n"); \
    } \
    if (i - 1 == cout.const_pool_size) printf("CONST POOL END-------\n"); \
  }

struct bytechunk {
  seal_byte* bytecodes; /* bytecode array */
  size_t size; /* bytecode size */
  size_t cap;  /* bytecode capacity */
};

struct const_pool {
  svalue_t *vals;
  size_t size;
  size_t cap;
};

struct label_pool {
  seal_word *addrs;
  size_t size;
  size_t cap;
};

struct scope {
  struct bytechunk bc;
  struct const_pool cp;
  struct label_pool lp;
  hashmap_t loctable;
};

struct cout {
  struct bytechunk bc;     
  svalue_t*  const_pool; /* pool for constant values */
  size_t const_pool_size;
  seal_word* labels; /* array of labels to store */
  size_t label_pool_size;
  size_t* skip_addr_offset_stack; /* stack for skip statements */
  size_t* stop_addr_offset_stack; /* stack for stop statements */
  size_t skip_size; /* skip statements size */
  size_t stop_size; /* stop statements size */
  seal_byte main_scope_local_size;
  struct scope main_scope;
};

void compile(cout_t*, ast_t*); /* init cout and compile root node into bytecode */
static void compile_node(cout_t*, ast_t*, struct scope*); /* compile any node into bytecode */
static void compile_if(cout_t*, ast_t*, struct scope*);
static void compile_while(cout_t*, ast_t*, struct scope*);
static void compile_dowhile(cout_t*, ast_t*, struct scope*);
static void compile_for(cout_t*, ast_t*, struct scope*);
static inline void compile_skip(cout_t*, struct scope*);
static inline void compile_stop(cout_t*, struct scope*);
static void compile_unary(cout_t*, ast_t*, struct scope*);
static void compile_binary(cout_t*, ast_t*, struct scope*);
static void compile_logical_binary(cout_t*, ast_t*, struct scope*);
static void compile_val(cout_t*, ast_t*, struct scope*);
static void compile_func_call(cout_t*, ast_t*, struct scope*);
static void compile_assign(cout_t*, ast_t*, struct scope*);
static void compile_var_ref(cout_t*, ast_t*, struct scope*);
static void compile_func_def(cout_t*, ast_t*, struct scope*);
static void compile_return(cout_t*, ast_t*, struct scope*);
static void compile_list(cout_t*, ast_t*, struct scope*);
static void compile_subscript(cout_t*, ast_t*, struct scope*);
static void compile_map(cout_t*, ast_t*, struct scope*);
static void compile_memacc(cout_t*, ast_t*, struct scope*);
static void compile_include(cout_t*, ast_t*, struct scope*);
static void compile_ternary(cout_t*, ast_t*, struct scope*);

#endif /* SEAL_COMPILER_H */
