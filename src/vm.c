#include "vm.h"
#include "builtins.h"
#include "gc.h"
#include "parser.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define FETCH(lf) (*lf->ip++)
#define PUSH(vm, val) do { \
  if (vm->sp - vm->stack == STACK_SIZE) \
    VM_ERROR("stack overflow"); \
  svalue_t loc_val = val; \
  *vm->sp++ = loc_val; \
  gc_incref(loc_val); \
} while (0)
#define DUP(vm) do { \
  svalue_t top = *(vm->sp - 1); \
  PUSH(vm, top); \
} while (0)
#define POP(vm) (*(--(vm->sp)))
#define JUMP(lf, addr) (lf->ip = &lf->bytecodes[lf->label_pool[addr]])
#define GET_CONST(lf, i) (lf->const_pool[i])
#define VM_ERROR(...) do { \
  int ___i = -1, ___line = -1; \
  while (++___i < lf->linfo_size) { \
    if (lf->ip - lf->bytecodes < lf->linfo[___i].offset) \
      break; \
    ___line = lf->linfo[___i].line; \
  } \
  fprintf(stderr, "seal: file: \'%s\', line %d\n", lf->file_name, ___line); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  exit(EXIT_FAILURE); \
} while (0)

#define ERROR_UNRY_OP(op, val) VM_ERROR("\'%s\' unary operator is not supported for \'%s\'", #op, seal_type_name(val.type))
#define ERROR_BIN_OP(op, left, right) VM_ERROR("\'%s\' operator is not supported for \'%s\' and \'%s\'", #op, seal_type_name(left.type), seal_type_name(right.type))

#define PUSH_NULL(vm)        PUSH(vm, (svalue_t) { .type = SEAL_NULL })
#define PUSH_INT(vm, val)    PUSH(vm, sval(SEAL_INT, _int, val))
#define PUSH_FLOAT(vm, val)  PUSH(vm, sval(SEAL_FLOAT, _float, val))
#define PUSH_STRING(vm, val) PUSH(vm, sval(SEAL_STRING, string, val))
#define PUSH_BOOL(vm, val)   PUSH(vm, sval(SEAL_BOOL, _bool, val))

#define TO_INT(val)
#define TO_FLOAT(val)
#define TO_STRING(val)
#define TO_BOOL(val) ( \
  IS_NULL(val) ? false : \
  IS_INT(val) ? AS_INT(val) != 0 : \
  IS_FLOAT(val) ? AS_FLOAT(val) != 0.0 : \
  IS_STRING(val) ? val.as.string->size > 0 : \
  IS_BOOL(val) ? AS_BOOL(val) : \
  IS_LIST(val) ? AS_LIST(val)->size > 0 : \
  IS_MAP(val) ? AS_MAP(val)->map->filled > 0 : \
  IS_FUNC(val) ? true : \
  IS_MOD(val) ? true : \
  IS_PTR(val) ? AS_PTR(val).ptr != NULL : \
  false \
)

/* macros for seal functions */
#define IS_FUNC_VARARG(val)    (AS_FUNC(val).is_vararg)
#define IS_BUILTIN_FUNC(val)   (AS_FUNC(val).type == FUNC_BUILTIN)
#define IS_USERDEF_FUNC(val)   (AS_FUNC(val).type == FUNC_USERDEF)
#define AS_BUILTIN_FUNC(val)   (AS_FUNC(val).as.builtin)
#define AS_USERDEF_FUNC(val)   (AS_FUNC(val).as.userdef)
#define FUNC_ARGC(val)         (IS_BUILTIN_FUNC(val) ? AS_BUILTIN_FUNC(val).argc : AS_USERDEF_FUNC(val).argc)
#define FUNC_NAME(val)         (AS_FUNC(val).name)
#define CALL_BUILTIN_FUNC(val) (AS_BUILTIN_FUNC(val).cfunc)

/* string manipulation */
struct seal_string* str_concat(const char* l, const char* r)
{
  int len = strlen(l) + strlen(r) + 1;
  char* res = SEAL_CALLOC(len, sizeof(char));
  strcpy(res, l);
  strcat(res, r);
  res[len - 1] = '\0';
  struct seal_string *str = SEAL_CALLOC(1, sizeof(struct seal_string));
  str->size = len - 1;
  str->is_static = false;
  str->ref_count = 0;
  str->val = res;
  return str;
}

/* arithmetic */
#define BIN_OP_INT(vm, left, right, op)   PUSH_INT(vm, AS_INT(left) op AS_INT(right))
#define BIN_OP_FLOAT(vm, left, right, op) PUSH_FLOAT(vm, AS_FLOAT(left) op AS_FLOAT(right))
#define BIN_OP_INT_AND_FLOAT(vm, left, right, op) \
  PUSH_FLOAT(vm, (IS_INT(left) ? AS_INT(left) : AS_FLOAT(left)) op (IS_INT(right) ? AS_INT(right) : AS_FLOAT(right)))

#define BIN_OP(vm, left, right, op) do { \
  if (IS_INT(left) && IS_INT(right)) \
    BIN_OP_INT(vm, left, right, op); \
  else if (IS_FLOAT(left) && IS_FLOAT(right)) \
    BIN_OP_FLOAT(vm, left, right, op); \
  else if (IS_INT(left) && IS_FLOAT(right) || IS_FLOAT(left) && IS_INT(right)) \
    BIN_OP_INT_AND_FLOAT(vm, left, right, op); \
  else if (IS_STRING(left) && IS_STRING(right)) \
    PUSH_STRING(vm, str_concat(AS_STRING(left), AS_STRING(right))); \
  else \
    ERROR_BIN_OP(op, left, right); \
} while (0)

#define MOD_OP(vm, left, right) do { \
  if (IS_INT(left) && IS_INT(right)) \
    PUSH_INT(vm, AS_INT(left) % AS_INT(right)); \
  else \
    ERROR_BIN_OP(%, left, right); \
} while (0)

/* bitwise binary */
#define BITWISE_OP(vm, left, right, op) do { \
  if (IS_INT(left) && IS_INT(right)) \
    PUSH_INT(vm, AS_INT(left) op AS_INT(right)); \
  else \
    ERROR_BIN_OP(op, left, right); \
} while (0)

/* equality */
#define EQUAL_OP_STRING(vm, left, right, op) PUSH_BOOL(vm, strcmp(AS_STRING(left), AS_STRING(right)) op 0)
#define EQUAL_OP_BOOL(vm, left, right, op)   PUSH_BOOL(vm, AS_BOOL(left) op AS_BOOL(right))
#define EQUAL_OP_INT(vm, left, right, op)    PUSH_BOOL(vm, AS_INT(left) op AS_INT(right))
#define EQUAL_OP_FLOAT(vm, left, right, op)  PUSH_BOOL(vm, AS_FLOAT(left) op AS_FLOAT(right))
#define EQUAL_OP_MAP(vm, left, right, op)    PUSH_BOOL(vm, AS_MAP(left)->map op AS_MAP(right)->map)
#define EQUAL_OP_NULL(vm, left, right, op)   PUSH_BOOL(vm, VAL_TYPE(left) op VAL_TYPE(right))
#define EQUAL_OP_INT_AND_FLOAT(vm, left, right, op) \
  PUSH_BOOL(vm, (IS_INT(left) ? AS_INT(left) : AS_FLOAT(left)) op (IS_INT(right) ? AS_INT(right) : AS_FLOAT(right)))

#define EQUAL_OP(vm, left, right, op) do { \
  if (IS_INT(left) && IS_INT(right)) \
    EQUAL_OP_INT(vm, left, right, op); \
  else if (IS_FLOAT(left) && IS_FLOAT(right)) \
    EQUAL_OP_FLOAT(vm, left, right, op); \
  else if (IS_INT(left) && IS_FLOAT(right) || IS_FLOAT(left) && IS_INT(right)) \
    EQUAL_OP_INT_AND_FLOAT(vm, left, right, op); \
  else if (IS_STRING(left) && IS_STRING(right)) \
    EQUAL_OP_STRING(vm, left, right, op); \
  else if (IS_BOOL(left) && IS_BOOL(right)) \
    EQUAL_OP_BOOL(vm, left, right, op); \
  else if (IS_MAP(left) && IS_MAP(right)) \
    EQUAL_OP_MAP(vm, left, right, op); \
  else if (IS_NULL(left) || IS_NULL(right)) \
    EQUAL_OP_NULL(vm, left, right, op); \
  else \
    ERROR_BIN_OP(op, left, right); \
} while (0)

/* comparison */
#define CMP_OP_INT(vm, left, right, op)    PUSH_BOOL(vm, AS_INT(left) op AS_INT(right))
#define CMP_OP_FLOAT(vm, left, right, op)  PUSH_BOOL(vm, AS_FLOAT(left) op AS_FLOAT(right))
#define CMP_OP_INT_AND_FLOAT(vm, left, right, op) \
  PUSH_BOOL(vm, (IS_INT(left) ? AS_INT(left) : AS_FLOAT(left)) op (IS_INT(right) ? AS_INT(right) : AS_FLOAT(right)))

#define CMP_OP(vm, left, right, op) do { \
  if (IS_INT(left) && IS_INT(right)) \
    CMP_OP_INT(vm, left, right, op); \
  else if (IS_FLOAT(left) && IS_FLOAT(right)) \
    CMP_OP_FLOAT(vm, left, right, op); \
  else if (IS_INT(left) && IS_FLOAT(right) || IS_FLOAT(left) && IS_INT(right)) \
    CMP_OP_INT_AND_FLOAT(vm, left, right, op); \
  else \
    ERROR_BIN_OP(op, left, right); \
} while (0)

/* unary */
#define UNRY_OP(vm, val, op) do { \
  switch (op) { \
  case OP_NEG: \
    if (IS_INT(val)) \
      PUSH_INT(vm, -(AS_INT(val))); \
    else if (IS_FLOAT(val)) \
      PUSH_FLOAT(vm, -(AS_FLOAT(val))); \
    else \
      ERROR_UNRY_OP(-, val); \
    break; \
  case OP_BNOT: \
    if (IS_INT(val)) \
      PUSH_INT(vm, ~(AS_INT(val))); \
    else \
      ERROR_UNRY_OP(~, val); \
    break; \
  case OP_NOT: \
    PUSH_BOOL(vm, !(TO_BOOL(val))); \
    break; \
  } \
} while (0)

/* local variables */
#define GET_LOCAL(lf, slot) (lf->locals[slot])
#define SET_LOCAL(lf, slot, val) (lf->locals[slot] = val)

/* initialization */
#define REGISTER_BUILTIN_FUNC(map, _name, str, _argc, _is_vararg) do { \
  svalue_t func = { \
    .type = SEAL_FUNC, \
    .as.func = { \
      .type = FUNC_BUILTIN, \
      .name = str, \
      .is_vararg = _is_vararg, \
      .as.builtin = { \
        .cfunc = _name, \
        .argc = _argc \
      } \
    } \
  }; \
  hashmap_insert(map, str, func); \
} while (0)

static hashmap_t mod_cache;

void init_mod_cache()
{
  hashmap_init(&mod_cache, 256);
}

static void set_all_functions_global(hashmap_t *globals)
{
  for (int i = 0; i < globals->cap; i++) {
    struct h_entry *e = &globals->entries[i];
    if (e->key && IS_FUNC(e->val) && IS_USERDEF_FUNC(e->val)) {
      AS_USERDEF_FUNC(e->val).globals = globals;
    }
  }
}

static void RUN_FILE(const char *path, hashmap_t *globals)
{
  int len = strlen(path) + 1;
  char* file_name = SEAL_CALLOC(len, sizeof(char));
  strcpy(file_name, path);

  ast_t* root;
  lexer_t lexer;
  init_lexer(&lexer, file_name);
  lexer_get_tokens(&lexer);
  parser_t parser;
  init_parser(&parser, &lexer);
  root = parser_parse(&parser);
  cout_t cout;
  compile(&cout, root, file_name);
  vm_t vm;
  init_vm(&vm, &cout);
  svalue_t locals[cout.main_scope_local_size];
  struct local_frame main_frame = {
    .locals = locals,
    .bytecodes = vm.bytecodes,
    .ip = vm.bytecodes,
    .label_pool = cout.labels,
    .const_pool = cout.const_pool,
    .globals = &vm.globals,
    .linfo = cout.bc.linfo,
    .linfo_size = cout.bc.l_size,
    .file_name = file_name,
  };
  *(globals) = vm.globals;
  eval_vm(&vm, &main_frame);
  *(globals) = vm.globals;
  set_all_functions_global(globals);
  //SEAL_FREE(vm.stack);
  //SEAL_FREE(vm.bytecodes);
  //SEAL_FREE(vm.label_ptr);
  //SEAL_FREE(vm.const_pool_ptr);
}

svalue_t insert_mod_cache(struct local_frame *lf, const char *name)
{
  struct h_entry *e = hashmap_search(&mod_cache, name);
  if (e->key)
    return e->val;

  int srch_curdir = 0;
  char path[256], *dir_name;
  FILE *f;
  const char *env = ".";
  svalue_t val = SEAL_VALUE_NULL;

  dir_name =
#ifdef _WIN32
  "Seal"
#else
  ".seal"
#endif
  "/lib/"
  ;

search:

  sprintf(path, "%s/%s%s.%s", env, srch_curdir ? dir_name : "", name,
#ifdef _WIN32
      "dll"
#else
      "so"
#endif
      );
  if ((f = fopen(path, "r")) != NULL) {
    fclose(f);
#ifdef _WIN32
    void *handle = LoadLibrary(path);
    if (!handle)
      VM_ERROR("failed to include \'%s\'", path);

    FARPROC function = GetProcAddress(handle, "seal_init_mod");
    val = ((svalue_t (*)()) function)();
#else
    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle)
      VM_ERROR("failed to include \'%s\'", path);

    val = ((svalue_t (*)()) dlsym(handle, "seal_init_mod"))();
#endif
    hashmap_insert_e(&mod_cache, e, name, val);
  } 

  if (IS_NULL(val)) {
    sprintf(path, "%s/%s%s.seal", env, srch_curdir ? dir_name : "", name);
    if ((f = fopen(path, "r")) != NULL) {
      fclose(f);
      val = (svalue_t) {
        .type = SEAL_MOD,
        .as.mod = SEAL_CALLOC(1, sizeof(struct seal_module))
      };
      val.as.mod->name = name;
      hashmap_insert_e(&mod_cache, e, name, val);
      val.as.mod->globals = SEAL_CALLOC(1, sizeof(hashmap_t));
      RUN_FILE(path, val.as.mod->globals);
    }
  }

  if (IS_NULL(val) && !srch_curdir) {
#ifdef _WIN32
    env = getenv("ProgramFiles");
#else
    env = getenv("HOME");
#endif
    srch_curdir = 1;
    goto search;
  }

  if (IS_NULL(val))
    VM_ERROR("including \'%s\' failed", path);

  return val;
}

/* static strings for typeof unary operator */
static svalue_t __seal_type_null,
                __seal_type_int,
                __seal_type_float,
                __seal_type_string,
                __seal_type_bool,
                __seal_type_list,
                __seal_type_map,
                __seal_type_func,
                __seal_type_mod,
                __seal_type_ptr;

#define TYPEOF_VAL_STR(val) ( \
  IS_NULL(val) ? __seal_type_null : \
  IS_INT(val) ? __seal_type_int : \
  IS_FLOAT(val) ? __seal_type_float : \
  IS_STRING(val) ? __seal_type_string : \
  IS_BOOL(val) ? __seal_type_bool : \
  IS_LIST(val) ? __seal_type_list : \
  IS_MAP(val) ? __seal_type_map : \
  IS_FUNC(val) ? __seal_type_func : \
  IS_MOD(val) ? __seal_type_mod : \
  IS_PTR(val) ? __seal_type_ptr : \
  SEAL_VALUE_NULL \
)
/********************************************/

void init_vm(vm_t* vm, cout_t* cout)
{
  vm->const_pool_ptr = cout->const_pool;
  vm->label_ptr = cout->labels;

  svalue_t *stack = SEAL_CALLOC(STACK_SIZE, sizeof(svalue_t));
  vm->stack = stack;

  vm->sp = vm->stack;
  vm->bytecodes = cout->bc.bytecodes;
  // vm->lf = SEAL_CALLOC(FRAME_MAX, sizeof(struct local_frame));
  // vm->lf[0] = (struct local_frame) { .ip = vm->bytecodes, .caller = NULL };
  hashmap_init(&vm->globals, 256);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_print, "print", 0, true);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_scan, "scan", 0, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_exit, "exit", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_len, "len", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_int, "int", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_float, "float", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_str, "str", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_bool, "bool", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_push, "push", 2, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_pop, "pop", 1, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_insert, "insert", 3, false);
  REGISTER_BUILTIN_FUNC(&vm->globals, __seal_remove, "remove", 2, false);


  __seal_type_null = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_NULL));
  __seal_type_int  = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_INT));
  __seal_type_float = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_FLOAT));
  __seal_type_string = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_STRING));
  __seal_type_bool = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_BOOL));
  __seal_type_list = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_LIST));
  __seal_type_map = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_MAP));
  __seal_type_func = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_FUNC));
  __seal_type_mod = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_MOD));
  __seal_type_ptr = SEAL_VALUE_STRING_STATIC(seal_type_name(SEAL_PTR));
}

void eval_vm(vm_t* vm, struct local_frame* lf)
{
  while (true) {
    seal_byte op = FETCH(lf);

    seal_word idx, addr;
    svalue_t  left, right;
    struct h_entry* entry;
    const char* sym;

    switch (op) {
    case OP_HALT:
      return;
    case OP_PUSH_CONST:
      idx = FETCH(lf) << 8;
      idx |= FETCH(lf);
      left = GET_CONST(lf, idx);
      if (IS_FUNC(left) && IS_USERDEF_FUNC(left)) {
        AS_USERDEF_FUNC(left).globals = lf->globals;
      }
      PUSH(vm, left);
      break;
    case OP_PUSH_INT:
      idx = FETCH(lf) << 8;
      idx |= FETCH(lf);
      PUSH_INT(vm, idx);
      break;
    case OP_PUSH_NULL:
      PUSH_NULL(vm);
      break;
    case OP_PUSH_TRUE:
      PUSH_BOOL(vm, true);
      break;
    case OP_PUSH_FALSE:
      PUSH_BOOL(vm, false);
      break;
    case OP_POP:
      left = POP(vm);
      gc_decref(left);
      break;
    case OP_DUP:
      DUP(vm);
      break;
    case OP_COPY:
      idx = FETCH(lf);
      PUSH(vm, *(vm->sp - idx));
      break;
    case OP_SWAP:
      idx = FETCH(lf);
      left = *(vm->sp - 1);
      *(vm->sp - 1) = *(vm->sp - idx);
      *(vm->sp - idx) = left;
      break;
    case OP_ADD:
      right = POP(vm);
      left  = POP(vm);
      BIN_OP(vm, left, right, +);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_SUB:
      right = POP(vm);
      left  = POP(vm);
      BIN_OP(vm, left, right, -);
      break;
    case OP_MUL:
      right = POP(vm);
      left  = POP(vm);
      BIN_OP(vm, left, right, *);
      break;
    case OP_DIV:
      right = POP(vm);
      left  = POP(vm);
      if (IS_NUM(right) && AS_NUM(right) == 0.0) {
        VM_ERROR("division by zero");
      }
      BIN_OP(vm, left, right, /);
      break;
    case OP_MOD:
      right = POP(vm);
      left  = POP(vm);
      MOD_OP(vm, left, right);
      break;
    case OP_AND:
      right = POP(vm);
      left  = POP(vm);
      BITWISE_OP(vm, left, right, &);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_OR:
      right = POP(vm);
      left  = POP(vm);
      BITWISE_OP(vm, left, right, |);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_XOR:
      right = POP(vm);
      left  = POP(vm);
      BITWISE_OP(vm, left, right, ^);
      break;
    case OP_SHL:
      right = POP(vm);
      left  = POP(vm);
      BITWISE_OP(vm, left, right, <<);
      break;
    case OP_SHR:
      right = POP(vm);
      left  = POP(vm);
      BITWISE_OP(vm, left, right, >>);
      break;
    case OP_EQ:
      right = POP(vm);
      left  = POP(vm);
      EQUAL_OP(vm, left, right, ==);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_NE:
      right = POP(vm);
      left  = POP(vm);
      EQUAL_OP(vm, left, right, !=);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_GT:
      right = POP(vm);
      left  = POP(vm);
      CMP_OP(vm, left, right, >);
      break;
    case OP_GE:
      right = POP(vm);
      left  = POP(vm);
      CMP_OP(vm, left, right, >=);
      break;
    case OP_LT:
      right = POP(vm);
      left  = POP(vm);
      CMP_OP(vm, left, right, <);
      break;
    case OP_LE:
      right = POP(vm);
      left  = POP(vm);
      CMP_OP(vm, left, right, <=);
      break;
    case OP_TYPOF:
      left = POP(vm);
      PUSH(vm, TYPEOF_VAL_STR(left));
      gc_decref(left);
      break;
    case OP_NOT:
    case OP_NEG:
    case OP_BNOT:
      left = POP(vm);
      UNRY_OP(vm, left, op);
      gc_decref(left);
      break;
    case OP_JUMP:
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      JUMP(lf, addr);
      break;
    case OP_JFALSE:
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      left = POP(vm);
      if (!TO_BOOL(left))
        JUMP(lf, addr);
      gc_decref(left);
      break;
    case OP_JTRUE:
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      left = POP(vm);
      if (TO_BOOL(left))
        JUMP(lf, addr);
      gc_decref(left);
      break;
    case OP_GET_GLOBAL:
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      sym = AS_STRING(GET_CONST(lf, addr));
      entry = hashmap_search(lf->globals, sym);
      if (entry && entry->key) {
        PUSH(vm, entry->val);
      } else {
        VM_ERROR("\'%s\' is not defined", sym);
      }
      break;
    case OP_SET_GLOBAL:
      DUP(vm);
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      hashmap_insert(lf->globals, AS_STRING(GET_CONST(lf, addr)), POP(vm));
      break;
    case OP_GET_LOCAL:
      addr = FETCH(lf);
      PUSH(vm, GET_LOCAL(lf, addr));
      break;
    case OP_SET_LOCAL:
      DUP(vm);
      addr = FETCH(lf);
      left = POP(vm);
      gc_decref(GET_LOCAL(lf, addr));
      SET_LOCAL(lf, addr, left);
      break;
    case OP_CALL: {
      seal_byte argc = FETCH(lf);
      svalue_t *argv = vm->sp - argc;
      vm->sp -= argc;

      svalue_t func = POP(vm);
      if (!IS_FUNC(func))
        VM_ERROR("calling non-function: \'%s\'", seal_type_name(func.type));

      if (!IS_FUNC_VARARG(func) && argc != FUNC_ARGC(func) || IS_FUNC_VARARG(func) && argc < FUNC_ARGC(func))
        VM_ERROR("\'%s\' function expected%s %d argument%s, got %d",
              FUNC_NAME(func),
              IS_FUNC_VARARG(func) ? " at least" : "",
              FUNC_ARGC(func),
              FUNC_ARGC(func) != 1 ? "s" : "",
              argc);

      if (IS_BUILTIN_FUNC(func)) {
        PUSH(vm, CALL_BUILTIN_FUNC(func)(argc, argv)); /* push function result to stack */
        for (int i = 0; i < argc; i++) {
          gc_decref(argv[i]);
        }
      } else {
        svalue_t locals[func.as.func.as.userdef.local_size];
        memset(locals, 0, sizeof(locals));
        struct local_frame func_lf = {
          .locals = locals,
          .ip = AS_USERDEF_FUNC(func).bytecode,
          .bytecodes = AS_USERDEF_FUNC(func).bytecode,
          .const_pool = AS_USERDEF_FUNC(func).const_pool,
          .label_pool = AS_USERDEF_FUNC(func).label_pool,
          .globals = AS_USERDEF_FUNC(func).globals ? AS_USERDEF_FUNC(func).globals : &vm->globals,
          .linfo = AS_USERDEF_FUNC(func).linfo,
          .linfo_size = AS_USERDEF_FUNC(func).linfo_size,
          .file_name = AS_USERDEF_FUNC(func).file_name,
        };
        for (int i = 0; i < argc; i++) {
          SET_LOCAL((&func_lf), i, argv[i]);
        }
        eval_vm(vm, &func_lf);
        for (int i = 0; i < func.as.func.as.userdef.local_size; i++) {
          gc_decref(locals[i]);
        }
      }
      break;
    }
    case OP_GEN_LIST: {
      seal_byte size = FETCH(lf);
      left = SEAL_VALUE_LIST();
      svalue_t sorted[size];
      for (int i = 0; i < size; i++) {
        sorted[i] = POP(vm);
      }
      for (int i = size - 1; i >= 0; i--) {
        LIST_PUSH(left, sorted[i]);
      }
      PUSH(vm, left);
      break;
    }
    case OP_GET_FIELD:
      right = POP(vm);
      left  = POP(vm);

      switch (VAL_TYPE(left)) {
      case SEAL_MAP: {
        if (!IS_STRING(right))
          VM_ERROR("map indices must be strings, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        struct sh_entry *e = shashmap_search(AS_MAP(left)->map, AS_STRING(right));
        if (e == NULL || e->key == NULL) {
          /* VM_ERROR("\'%s\' key is not found", AS_STRING(right)); */
          PUSH(vm, SEAL_VALUE_NULL);
        } else {
          PUSH(vm, e->val);
        }

        break;
      }
      case SEAL_MOD: {
        if (!IS_STRING(right))
          VM_ERROR("module indices must be strings, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        struct h_entry *e = hashmap_search(AS_MOD(left)->globals, AS_STRING(right));
        if (e == NULL || e->key == NULL)
          VM_ERROR("\'%s\' module has no field named \'%s\'", AS_MOD(left)->name, AS_STRING(right));

        PUSH(vm, e->val);

        break;
      }
      case SEAL_STRING: {
        if (!IS_INT(right))
          VM_ERROR("string indices must be integers, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        if (AS_INT(right) >= left.as.string->size || AS_INT(right) < 0)
          VM_ERROR("string index out of range");

        char *c = SEAL_CALLOC(2, sizeof(char));
        c[0] = AS_STRING(left)[AS_INT(right)];
        c[1] = '\0';
        PUSH(vm, SEAL_VALUE_STRING(c));

        break;
      }
      case SEAL_LIST: {
        if (!IS_INT(right))
          VM_ERROR("list indices must be integers, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        if (AS_INT(right) >= AS_LIST(left)->size || AS_INT(right) < 0)
          VM_ERROR("list index out of range");

        PUSH(vm, AS_LIST(left)->mems[AS_INT(right)]);

        break;
      }
      default:
        VM_ERROR("cannot index \'%s\'", seal_type_name(VAL_TYPE(left)));
        break;
      }

      gc_decref(left);
      gc_decref(right);

      break;
    case OP_SET_FIELD:
      right = POP(vm);
      left  = POP(vm);

      switch (VAL_TYPE(left)) {
      case SEAL_MAP: {
        if (!IS_STRING(right))
          VM_ERROR("map indices must be strings, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        struct sh_entry *e = shashmap_search(AS_MAP(left)->map, AS_STRING(right));
        if (e == NULL)
          VM_ERROR("cannot insert, map is full");

        if (e->key != NULL)
          gc_decref(e->val);
        else
          AS_MAP(left)->map->filled++;

        e->val = POP(vm);

        e->key = AS_STRING(right);
        e->hash = shash_str(AS_STRING(right));
        e->is_tombstone = true;

        PUSH(vm, e->val);

        break;
      }
      case SEAL_LIST: {
        if (!IS_INT(right))
          VM_ERROR("list indices must be integers, not \'%s\'", seal_type_name(VAL_TYPE(right)));

        if (AS_INT(right) >= AS_LIST(left)->size || AS_INT(right) < 0)
          VM_ERROR("list index out of range");

        gc_decref(AS_LIST(left)->mems[AS_INT(right)]);
        PUSH(vm, AS_LIST(left)->mems[AS_INT(right)] = POP(vm));

        break;
      }
      case SEAL_STRING: case SEAL_MOD:
        VM_ERROR("%ss are immutable", seal_type_name(VAL_TYPE(left)));
        break;
      default:
        VM_ERROR("cannot index \'%s\'", seal_type_name(VAL_TYPE(left)));
        break;
      }

      gc_decref(left);
      gc_decref(right);

      break;
    case OP_IN:
      right = POP(vm);
      left  = POP(vm);
      if (!IS_STRING(right))
        VM_ERROR("in operator requires string");
      if (!IS_STRING(left))
        VM_ERROR("leftside must be string when rightside is string");

      PUSH_BOOL(vm, strstr(AS_STRING(right), AS_STRING(left)) != NULL);
      gc_decref(left);
      gc_decref(right);
      break;
    case OP_GEN_MAP: {
      seal_byte size = FETCH(lf);
      left = SEAL_VALUE_MAP();
      for (int i = 0; i < size; i++) {
        const char *key = AS_STRING(POP(vm));
        right = POP(vm);


        MAP_INSERT(left, key, right); 
      }
      PUSH(vm, left);
      break;
    }
    case OP_INCLUDE:
      left = POP(vm);
      PUSH(vm, insert_mod_cache(lf, AS_STRING(left)));
      break;
    case OP_INCLUDE_SYM: {
      seal_byte size = FETCH(lf);
      const char *names[size];
      for (int i = size - 1; i >= 0; i--) {
        names[i] = AS_STRING(POP(vm));
      }
      left = POP(vm); /* module */
      for (int i = 0; i < size; i++) {
        struct h_entry *e = hashmap_search(AS_MOD(left)->globals, names[i]);
        if (e == NULL || e->key == NULL)
          VM_ERROR("failed to load \'%s\' symbol from \'%s\'", names[i], AS_MOD(left)->name);

        hashmap_insert(lf->globals, names[i], e->val);
      }
      break;
   }
    case OP_FOR_PREP:
      /*
       * *(sp - 1) -> iterator (in integer)
       * *(sp - 2) -> step (in integer)
       * *(sp - 3) -> iterated (generic)
       *
       * subtract step from iterator before loop
       * then jump to FOR_NEXT instruction
       */
      AS_INT(*(vm->sp - 1)) -= AS_INT(*(vm->sp - 2));
      addr = FETCH(lf) << 8;
      addr |= FETCH(lf);
      JUMP(lf, addr);
      break;
    case OP_FOR_NEXT:
      left = *(vm->sp - 3);
      AS_INT(*(vm->sp - 1)) += AS_INT(*(vm->sp - 2));
      switch (VAL_TYPE(left)) {
      case SEAL_INT:
        if (AS_INT(*(vm->sp - 1)) >= AS_INT(left)) {
          goto finish_loop;
        } else {
          SET_LOCAL(lf, FETCH(lf), *(vm->sp - 1));
          addr = FETCH(lf) << 8;
          addr |= FETCH(lf);
          JUMP(lf, addr);
        }
        break;
      case SEAL_STRING:
        if (AS_INT(*(vm->sp - 1)) >= left.as.string->size) {
          goto finish_loop;
        } else {
          char *c = SEAL_CALLOC(2, sizeof(char));
          c[0] = AS_STRING(left)[AS_INT(*(vm->sp - 1))];
          c[1] = '\0';
          right = SEAL_VALUE_STRING(c);
          gc_incref(right);
          idx = FETCH(lf);
          gc_decref(GET_LOCAL(lf, idx));
          SET_LOCAL(lf, idx, right);
          addr = FETCH(lf) << 8;
          addr |= FETCH(lf);
          JUMP(lf, addr);
        }
        break;
      case SEAL_LIST:
        if (AS_INT(*(vm->sp - 1)) >= AS_LIST(left)->size) {
          goto finish_loop;
        } else {
          right = AS_LIST(left)->mems[AS_INT(*(vm->sp - 1))];
          gc_incref(right);
          idx = FETCH(lf);
          gc_decref(GET_LOCAL(lf, idx));
          SET_LOCAL(lf, idx, right);
          addr = FETCH(lf) << 8;
          addr |= FETCH(lf);
          JUMP(lf, addr);
        }
        break;
      }
      break;
finish_loop:
      gc_decref(*(vm->sp - 3));
      vm->sp -= 3;
      lf->ip += 3;
      break;
    case OP_FOR_STOP:
      gc_decref(*(vm->sp - 3));
      vm->sp -= 3;
      break;
    default:
      fprintf(stderr, "unrecognized op type: %d\n", op);
      return;
    }
  }
}
