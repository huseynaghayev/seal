#include <seal.h>
#include <time.h>

static const char *MOD_NAME = "date";

seal_value __seal_date_time(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "ascii";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  time_t timer;
  time(&timer);
  return SEAL_VALUE_INT(timer);
}
seal_value seal_init_mod()
{
  seal_value mod = {
    .type = SEAL_MOD,
    .as.mod = SEAL_CALLOC(1, sizeof(struct seal_module))
  };
  AS_MOD(mod)->name = MOD_NAME;
  AS_MOD(mod)->globals = SEAL_CALLOC(1, sizeof(hashmap_t));
  hashmap_init(AS_MOD(mod)->globals, 1);

  MOD_REGISTER_FUNC(mod, __seal_date_time, "time", 0, false);

  return mod;
}
