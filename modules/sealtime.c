#include <seal.h>
#include <time.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif


static const char *MOD_NAME = "time";


seal_value __seal_time_now(seal_byte argc, seal_value *argv)
{
  time_t timer;
  time(&timer);
  return SEAL_VALUE_INT(timer);
}

seal_value __seal_time_sleep(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "sleep";

  seal_value dur;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_INT), &dur);

#ifdef _WIN32
  Sleep(AS_INT(dur) * 1000); /* in milliseconds */
#else
  sleep(AS_INT(dur));
#endif
  return SEAL_VALUE_NULL;
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

  MOD_REGISTER_FUNC(mod, __seal_time_now,   "now",   0, false);
  MOD_REGISTER_FUNC(mod, __seal_time_sleep, "sleep", 1, false);

  return mod;
}
