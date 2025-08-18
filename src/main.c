#include "ast.h"
#include "sealconf.h"

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "gc.h"

#define USAGE(prog_name) (fprintf(stdout, "seal: usage: %s filename.seal\n", prog_name))
#define PRINT_FLAGS() (fprintf(stderr, "seal: flags: -pt (print tokens), -pa (print AST), -po (print opcodes), -pb (print bytes), -pc (print constant pool)\n"))
#define PRINT_VERSION() (fprintf(stdout, "Seal %s\n", VERSION))

int main(int argc, char** argv)
{
  if (argc < 2) {
    USAGE(argv[0]);
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
    PRINT_VERSION();
    return EXIT_SUCCESS;
  }

  /* FLAGS */
  bool PRINT_TOKS = false;
  bool PRINT_AST  = false;
  bool PRINT_OP   = false;
  bool PRINT_BYTE = false;
  bool PRINT_CONST_POOL = false;
  bool PRINT_STACK = false;

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-pt") == 0) {
      PRINT_TOKS = true;
    } else if (strcmp(argv[i], "-pa") == 0) {
      PRINT_AST = true;
    } else if (strcmp(argv[i], "-po") == 0) {
      PRINT_OP  = true;
    } else if (strcmp(argv[i], "-pb") == 0) {
      PRINT_BYTE = true;
    } else if (strcmp(argv[i], "-pc") == 0) {
      PRINT_CONST_POOL = true;
    } else if (strcmp(argv[i], "-ps") == 0) {
      PRINT_STACK = true;
    }
  }
  const char* file_path = argv[1];
  /* lexing */
  ast_t* root;
  lexer_t lexer;
  init_lexer(&lexer, file_path);
  lexer_get_tokens(&lexer);
  if (PRINT_TOKS)
    print_tokens(lexer.toks, lexer.tok_size);
  /* parsing and generating abstract syntax tree (AST) */
  
  create_const_asts(); /* allocate constant ASTs */
  parser_t parser;
  init_parser(&parser, &lexer);
  root = parser_parse(&parser);
  if (PRINT_AST)
    print_ast(root);

  cout_t cout;
  compile(&cout, root, parser.file_path);
  if (PRINT_OP)
    print_op(cout.bc.bytecodes, cout.bc.size, cout.labels, cout.label_pool_size);
  if (PRINT_BYTE)
    PRINT_BYTE(cout.bc.bytecodes, cout.bc.size)
  if (PRINT_CONST_POOL)
    PRINT_CONST_POOL(cout);

  init_mod_cache(); /* initalize modules cache before running main file */

  vm_t vm;
  init_vm(&vm, &cout);
  svalue_t locals[cout.main_scope_local_size];
  memset(locals, 0, sizeof(locals));
  struct local_frame main_frame = {
    .locals = locals,
    .bytecodes = vm.bytecodes,
    .ip = vm.bytecodes,
    .label_pool = cout.labels,
    .const_pool = cout.const_pool,
    .globals = &vm.globals,
    .linfo = cout.bc.linfo,
    .linfo_size = cout.bc.l_size,
    .file_name = file_path,
  };
  /* add 'args' global variable as command line args' */
  svalue_t list_args = SEAL_VALUE_LIST();
  gc_incref(list_args);
  for (int i = 1; i < argc; i++) {
    char *alloc_s = SEAL_CALLOC(strlen(argv[i]) + 1, sizeof(char));
    strcpy(alloc_s, argv[i]);
    svalue_t seal_str = SEAL_VALUE_STRING(alloc_s);
    gc_incref(seal_str);
    LIST_PUSH(list_args, seal_str);
  }
  hashmap_insert(&vm.globals, "args", list_args);

  eval_vm(&vm, &main_frame);
  if (PRINT_STACK)
    print_stack(&vm);

  //svalue_t func = hashmap_search(&vm.globals, "add")->val;
  //print_op(func.as.func.as.userdef.bytecode, 36, cout.labels, 0);

  /* FREE EVERYTHING AFTER BEING USED */
  
  return EXIT_SUCCESS;
}
