#include "builtins.h"
#include "gc.h"

#define BUILTIN_ERROR(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  exit(1); \
} while (0)

#define BUILTIN_FUNC_ARG_ERR(name, arg_i, given, expected) do { \
  BUILTIN_ERROR("in function \'%s\': expected arg %d to be \'%s\', got \'%s\'", \
                name, arg_i, seal_type_name(expected), seal_type_name(given)); \
} while (0)

static void __print_string_no_escseq(const char *s)
{
  while (*s) {
    switch (*s) {
    case '\\':
      printf("%s", "\\\\");
      break;
    case '\n':
      printf("%s", "\\n");
      break;
    case '\t':
      printf("%s", "\\t");
      break;
    case '\'':
      printf("%s", "\\'");
      break;
    case '\"':
      printf("%s", "\\\"");
      break;
    case '\b':
      printf("%s", "\\b");
      break;
    default:
      printf("%c", *s);
    }
    s++;
  }
}

static void __print_single(svalue_t s)
{
  switch (s.type) {
  case SEAL_INT:
    printf("%lld", AS_INT(s));
    break;
  case SEAL_FLOAT:
    if (AS_FLOAT(s) == (seal_int)AS_FLOAT(s))
      printf("%.1f", AS_FLOAT(s));
    else
      printf("%.16g", AS_FLOAT(s));
    break;
  case SEAL_STRING:
    printf("%s", AS_STRING(s));
    break;
  case SEAL_BOOL:
    printf("%s", AS_BOOL(s) ? "true" : "false");
    break;
  case SEAL_NULL:
    printf("null");
    break;
  case SEAL_FUNC:
    printf("function: %p", AS_FUNC(s).type == FUNC_BUILTIN ? (void*)AS_FUNC(s).as.builtin.cfunc : (void*)AS_FUNC(s).as.userdef.bytecode);
    break;
  case SEAL_LIST:
    printf("[");
    for (int i = 0; i < AS_LIST(s)->size; i++) {
      svalue_t mem = AS_LIST(s)->mems[i];

      if (IS_STRING(mem))
        printf("\'");

      if (IS_LIST(mem) && AS_LIST(s) == AS_LIST(mem))
        printf("[...]");
      else if (IS_STRING(mem))
        __print_string_no_escseq(AS_STRING(mem));
      else
        __print_single(mem);

      if (IS_STRING(mem))
        printf("\'");

      if (i < AS_LIST(s)->size - 1)
        printf(", ");
    }
    printf("]");
    break;
  case SEAL_MAP: {
    int left = AS_MAP(s)->map->filled;
    printf("{");
    for (int i = 0; i < AS_MAP(s)->map->cap; i++) {
      struct sh_entry e = AS_MAP(s)->map->entries[i];
      if (e.key == NULL)
        continue;
      left--;
      printf("%s: ", e.key);
      if (IS_STRING(e.val)) {
        printf("\'");
        __print_string_no_escseq(AS_STRING(e.val));
        printf("\'");
      } else {
        __print_single(e.val);
      }


      if (left)
        printf(", ");
    }
    printf("}");
    break;
  }
  case SEAL_MOD:
    printf("module: %s (%p)", s.as.mod->name, s.as.mod->globals);
    break;
  case SEAL_PTR:
    printf("%s: %p", s.as.ptr.name, s.as.ptr.ptr);
    break;
  default:
    printf("UNRECOGNIZED DATA TYPE TO PRINT ");
  }
}
svalue_t __seal_print(seal_byte argc, svalue_t* argv)
{
  for (int i = 0; i < argc; i++) {
    svalue_t s = argv[i];

    __print_single(s);
    if (i < argc - 1)
      printf(" ");
  }
  printf("\n");

  return SEAL_VALUE_NULL;
}
svalue_t __seal_scan(seal_byte argc, svalue_t* argv)
{
  static char buf[BUFSIZ];
  size_t len = 0;
  while ((buf[len] = getchar()) != EOF && buf[len] != '\n') {
    len++;
  }
  buf[len] = '\0';
  char* s = SEAL_CALLOC(len, sizeof(char));
  strcpy(s, buf);
  return SEAL_VALUE_STRING(s);
}
svalue_t __seal_exit(seal_byte argc, svalue_t* argv)
{
   if (!IS_INT(argv[0]))
    BUILTIN_FUNC_ARG_ERR("exit", 0, VAL_TYPE(argv[0]), SEAL_INT); 

   exit(AS_INT(argv[0]));
   return SEAL_VALUE_NULL;
}
svalue_t __seal_len(seal_byte argc, svalue_t* argv)
{
  if (!IS_STRING(argv[0]) && !IS_LIST(argv[0]))
    BUILTIN_FUNC_ARG_ERR("len", 0, VAL_TYPE(argv[0]), SEAL_STRING | SEAL_LIST);

  return SEAL_VALUE_INT(IS_STRING(argv[0]) ? argv[0].as.string->size : AS_LIST(argv[0])->size);
}
svalue_t __seal_int(seal_byte argc, svalue_t* argv)
{
  if (!IS_STRING(argv[0]))
    BUILTIN_FUNC_ARG_ERR("int", 0, VAL_TYPE(argv[0]), SEAL_STRING);

  return SEAL_VALUE_INT(atoi(AS_STRING(argv[0])));
}
svalue_t __seal_float(seal_byte argc, svalue_t* argv)
{
  svalue_t s = argv[0];
  
  switch (s.type) {
  case SEAL_INT:
    return SEAL_VALUE_FLOAT(AS_INT(s));
  case SEAL_FLOAT:
    return s;
  case SEAL_STRING:
    return SEAL_VALUE_FLOAT(atof(AS_STRING(s)));
  }

  return SEAL_VALUE_NULL;
}
svalue_t __seal_push(seal_byte argc, svalue_t* argv)
{
  if (!IS_LIST(argv[0]))
    BUILTIN_FUNC_ARG_ERR("push", 0, VAL_TYPE(argv[0]), SEAL_LIST);

  LIST_PUSH(argv[0], argv[1]);
  gc_incref(argv[1]);
  return SEAL_VALUE_NULL;
}
svalue_t __seal_pop(seal_byte argc, svalue_t* argv)
{
  if (!IS_LIST(argv[0]))
    BUILTIN_FUNC_ARG_ERR("pop", 0, VAL_TYPE(argv[0]), SEAL_LIST);

  if (AS_LIST(argv[0])->size == 0)
    BUILTIN_ERROR("cannot pop empty list");

  svalue_t popped = AS_LIST(argv[0])->mems[--AS_LIST(argv[0])->size];
  gc_decref_nofree(popped);
  return popped;
}
