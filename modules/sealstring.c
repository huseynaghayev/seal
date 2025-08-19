#include <seal.h>


static const char *MOD_NAME = "string";


seal_value __seal_string_ascii(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "ascii";

  seal_value str;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &str);

  return SEAL_VALUE_INT((seal_int) *AS_STRING(str));
}
seal_value __seal_string_char(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "char";

  seal_value i;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_INT), &i);

  char *c = SEAL_CALLOC(2, sizeof(char));
  c[0] = AS_INT(i);
  c[1] = '\0';
  return SEAL_VALUE_STRING(c);
}
seal_value seal_init_mod()
{
  seal_value mod = {
    .type = SEAL_MOD,
    .as.mod = SEAL_CALLOC(1, sizeof(struct seal_module))
  };
  AS_MOD(mod)->name = MOD_NAME;
  AS_MOD(mod)->globals = SEAL_CALLOC(1, sizeof(hashmap_t));
  hashmap_init(AS_MOD(mod)->globals, 8);

  MOD_REGISTER_FUNC(mod, __seal_string_ascii, "ascii", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_string_char,  "char",  1, false);

  return mod;
}
