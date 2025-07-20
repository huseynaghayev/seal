#include "builtins.h"
#include "gc.h"
#include "moddef.h"

#define BUILTIN_ERROR(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  exit(1); \
} while (0)

static const char *MOD_NAME = "<built-in>";

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
  static const char *FUNC_NAME = "exit";

  svalue_t exit_code;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &exit_code);

  exit(AS_INT(exit_code));
  return SEAL_VALUE_NULL;
}
svalue_t __seal_len(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "len";

  svalue_t it;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_STRING | SEAL_LIST), &it);

  return SEAL_VALUE_INT(IS_STRING(it) ? it.as.string->size : AS_LIST(it)->size);
}
svalue_t __seal_int(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "int";

  svalue_t arg;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_STRING | SEAL_NUMBER), &arg);

  return SEAL_VALUE_INT(IS_STRING(arg) ? atoi(AS_STRING(arg)) : AS_NUM(arg));
}
svalue_t __seal_float(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "float";

  svalue_t arg;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_STRING | SEAL_NUMBER), &arg);

  return SEAL_VALUE_FLOAT(IS_STRING(arg) ? atof(AS_STRING(arg)) : AS_NUM(arg));
}
svalue_t __seal_str(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "str";

  svalue_t arg;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_NULL | SEAL_INT | SEAL_FLOAT | SEAL_STRING | SEAL_BOOL), &arg);

  svalue_t res;
  char *s;
  int len;

  switch (VAL_TYPE(arg)) {
  case SEAL_NULL:
    s = malloc(5 * sizeof(char));
    strcpy(s, "null");
    res = SEAL_VALUE_STRING(s);
    break;
  case SEAL_INT:
    len = snprintf(NULL, 0, "%lld", AS_INT(arg));
    s = malloc(len * sizeof(char));
    sprintf(s, "%lld", AS_INT(arg));
    res = SEAL_VALUE_STRING(s);
    break;
  case SEAL_FLOAT:
    len = snprintf(NULL, 0, "%g", AS_FLOAT(arg));
    s = malloc(len * sizeof(char));
    sprintf(s, "%g", AS_FLOAT(arg));
    res = SEAL_VALUE_STRING(s);
    break;
  case SEAL_STRING:
    return arg;
    break;
  case SEAL_BOOL:
    if (AS_BOOL(arg)) {
      s = malloc(5 * sizeof(char));
      strcpy(s, "true");
      res = SEAL_VALUE_STRING(s);
    } else {
      s = malloc(6 * sizeof(char));
      strcpy(s, "false");
      res = SEAL_VALUE_STRING(s);
    }
    break;
  }

  return res;
}
svalue_t __seal_bool(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "bool";

  svalue_t arg;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_ANY), &arg);

  return SEAL_VALUE_BOOL(
      IS_NULL(arg) ? false :
      IS_INT(arg) ? AS_INT(arg) != 0 :
      IS_FLOAT(arg) ? AS_FLOAT(arg) != 0.0 :
      IS_STRING(arg) ? arg.as.string->size > 0 :
      IS_BOOL(arg) ? AS_BOOL(arg) :
      IS_LIST(arg) ? AS_LIST(arg)->size > 0 :
      IS_MAP(arg) ? AS_MAP(arg)->map->filled > 0 :
      IS_FUNC(arg) ? true :
      IS_MOD(arg) ? true :
      IS_PTR(arg) ? AS_PTR(arg).ptr != NULL :
      false);
}
svalue_t __seal_push(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "push";

  svalue_t list, e;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 2, PARAM_TYPES(SEAL_LIST, SEAL_ANY), &list, &e);

  LIST_PUSH(list, e);
  gc_incref(e);

  return SEAL_VALUE_NULL;
}
svalue_t __seal_pop(seal_byte argc, svalue_t* argv)
{
  static const char *FUNC_NAME = "pop";

  svalue_t list;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_LIST), &list);

  if (AS_LIST(argv[0])->size == 0)
    BUILTIN_ERROR("cannot pop empty list");

  svalue_t popped = AS_LIST(list)->mems[--AS_LIST(list)->size];
  gc_decref_nofree(popped);
  return popped;
}
svalue_t __seal_insert(seal_byte argc, svalue_t *argv)
{
  static const char *FUNC_NAME = "insert";

  svalue_t list, idx, e;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 3, PARAM_TYPES(SEAL_LIST, SEAL_INT, SEAL_ANY), &list, &idx, &e);


  struct seal_list *l = AS_LIST(list);
  if (l->size >= l->cap) {
    l->mems = SEAL_REALLOC(l->mems, sizeof(svalue_t) * (l->cap *= 2));
  }

  int clamped_idx = AS_INT(idx) < 0 ? 0 : (AS_INT(idx) > l->size ? l->size : AS_INT(idx));
  
  for (int i = l->size; i > clamped_idx; i--)
    l->mems[i] = l->mems[i - 1];

  l->size++;

  l->mems[clamped_idx] = e;
  gc_incref(e);

  return SEAL_VALUE_NULL;
}
svalue_t __seal_remove(seal_byte argc, svalue_t *argv)
{
  static const char *FUNC_NAME = "remove";

  svalue_t list, idx;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 2, PARAM_TYPES(SEAL_LIST, SEAL_INT), &list, &idx);

  struct seal_list *l = AS_LIST(list);
  svalue_t removed;

  int clamped_idx = AS_INT(idx) < 0 ? 0 : (AS_INT(idx) >= l->size ? l->size - 1 : AS_INT(idx));

  removed = l->mems[clamped_idx];
  
  for (int i = clamped_idx; i < l->size; i++)
    l->mems[i] = l->mems[i + 1];

  l->size--;

  gc_decref_nofree(removed);

  return removed;
}
