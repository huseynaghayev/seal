#include "moddef.h"


void seal_parse_args(const char *MOD_NAME,
                     const char *FUNC_NAME,
                     seal_byte argc,
                     svalue_t *argv,
                     seal_byte typec,
                     int *typev,
                     ...)
{
  if (argc != typec)
    MOD_ERROR("expected %d argument%s, got %d", typec, typec != 1 ? "s" : "", argc);

  va_list vargs;
  va_start(vargs, typev);

  for (int i = 0; i < typec; i++) {

    svalue_t *ps = va_arg(vargs, svalue_t*);

    *ps = argv[i];

    int type = VAL_TYPE(argv[i]);
    if (!(type & typev[i])) {
      MOD_ERROR("expected argument %d to be \'%s\', got \'%s\'",
                i,
                typev[i] == SEAL_PTR ? va_arg(vargs, const char*) : seal_type_name(typev[i]),
                seal_type_name(type));
    } else if (type == SEAL_PTR) {
      const char *ptr_name = va_arg(vargs, const char*);
      if (!STR_EQ(ptr_name, AS_PTR(*ps).name))
        MOD_ERROR("expected argument %d to be \'%s\', got \'%s\'",
                  i,
                  AS_PTR(*ps).name,
                  ptr_name);
    }
  }

  va_end(vargs);
}

svalue_t seal_map_get_field(const char *mod_name, const char *func_name, svalue_t *map, const char *key)
{
  return SEAL_VALUE_NULL;
}
