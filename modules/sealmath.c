#include <seal.h>
#include <math.h>


static const char *MOD_NAME = "math";


seal_value __seal_math_sqrt(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "sqrt";

  seal_value num;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &num);

  return SEAL_VALUE_FLOAT(sqrt(AS_NUM(num)));
}

seal_value __seal_math_cbrt(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "cbrt";

  seal_value num;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &num);

  return SEAL_VALUE_FLOAT(cbrt(AS_NUM(num)));
}

seal_value __seal_math_pow(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "pow";

  seal_value base, exp;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_NUMBER, SEAL_NUMBER), &base, &exp);

  return SEAL_VALUE_FLOAT(pow(AS_NUM(base), AS_NUM(exp)));
}

seal_value __seal_math_sin(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "sin";

  seal_value rad;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &rad);

  return SEAL_VALUE_FLOAT(sin(AS_NUM(rad)));
}

seal_value __seal_math_cos(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "cos";

  seal_value rad;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &rad);

  return SEAL_VALUE_FLOAT(cos(AS_NUM(rad)));
}

seal_value __seal_math_tan(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "tan";

  seal_value rad;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &rad);

  return SEAL_VALUE_FLOAT(tan(AS_NUM(rad)));
}

seal_value __seal_math_asin(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "asin";

  seal_value val;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &val);

  return SEAL_VALUE_FLOAT(asin(AS_NUM(val)));
}

seal_value __seal_math_acos(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "acos";

  seal_value val;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &val);

  return SEAL_VALUE_FLOAT(acos(AS_NUM(val)));
}

seal_value __seal_math_atan(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "atan";

  seal_value val;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_NUMBER), &val);

  return SEAL_VALUE_FLOAT(atan(AS_NUM(val)));
}

seal_value __seal_math_atan2(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "atan2";

  seal_value y, x;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_NUMBER, SEAL_NUMBER), &y, &x);

  return SEAL_VALUE_FLOAT(atan2(AS_NUM(y), AS_NUM(x)));
}

seal_value seal_init_mod()
{
  seal_value mod = {
    .type = SEAL_MOD,
    .as.mod = SEAL_CALLOC(1, sizeof(struct seal_module))
  };
  AS_MOD(mod)->name = MOD_NAME;
  AS_MOD(mod)->globals = SEAL_CALLOC(1, sizeof(hashmap_t));
  hashmap_init(AS_MOD(mod)->globals, 48);

  MOD_REGISTER_FUNC(mod, __seal_math_sqrt,  "sqrt",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_cbrt,  "cbrt",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_pow,   "pow",    2, false);
  MOD_REGISTER_FUNC(mod, __seal_math_sin,   "sin",    1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_cos,   "cos",    1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_tan,   "tan",    1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_asin,  "asin",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_acos,  "acos",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_atan,  "atan",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_math_atan2, "atan2",  2, false);

  return mod;
}
