#include <seal.h>
#include <stdlib.h>
#include <limits.h>
#ifdef _WIN32
/* include windows headers */
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif


static const char *MOD_NAME = "os";


#define STR_HCOPY(dst, src) do { \
  dst = SEAL_CALLOC(strlen(src) + 1, sizeof(char)); \
  strcpy(dst, src); \
} while (0)

seal_value __seal_os_system(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "system";

  seal_value command;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &command);

  return SEAL_VALUE_INT(system(AS_STRING(command)));
}

seal_value __seal_os_getenv(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "getenv";

  seal_value name;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &name);

  const char *searched = getenv(AS_STRING(name));
  if (!searched)
    return SEAL_VALUE_NULL;

  char *copied;
  STR_HCOPY(copied, searched);
  return SEAL_VALUE_STRING(copied);
}

seal_value __seal_os_setenv(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "setenv";

  seal_value name, value, overwrite;
  SEAL_PARSE_ARGS(3, PARAM_TYPES(SEAL_STRING, SEAL_STRING, SEAL_BOOL), &name, &value, &overwrite);

  int res = setenv(AS_STRING(name), AS_STRING(value), AS_BOOL(overwrite));
  return SEAL_VALUE_BOOL(res == 0);
}

seal_value __seal_os_unsetenv(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "unsetenv";

  seal_value name;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &name);

  int res = unsetenv(AS_STRING(name));
  return SEAL_VALUE_BOOL(res == 0);
}

seal_value __seal_os_getcwd(seal_byte argc, seal_value *argv)
{
  char buf[512];
  if (getcwd(buf, 512) == NULL)
    return SEAL_VALUE_NULL;

  char *copied;
  STR_HCOPY(copied, buf);
  return SEAL_VALUE_STRING(copied);
}

seal_value __seal_os_chdir(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "chdir";

  seal_value dir;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &dir);

  return SEAL_VALUE_BOOL(chdir(AS_STRING(dir)) == 0);
}

seal_value __seal_os_readdir(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "readdir";

  seal_value path;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &path);

  DIR *dp = opendir(AS_STRING(path)); 

  if (dp == NULL)
    return SEAL_VALUE_NULL;

  struct dirent *de;
  char *file_name;
  seal_value files_list = SEAL_VALUE_LIST();

  while ((de = readdir(dp)) != NULL) {
    if (STR_EQ(de->d_name, ".") || STR_EQ(de->d_name, ".."))
      continue;

    STR_HCOPY(file_name, de->d_name);
    seal_value seal_file_name = SEAL_VALUE_STRING(file_name);
    gc_incref(seal_file_name);
    LIST_PUSH(files_list, seal_file_name);
  }

  closedir(dp);

  return files_list;
}

seal_value __seal_os_remove(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "remove";

  seal_value path;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &path);

  return SEAL_VALUE_BOOL(remove(AS_STRING(path)) == 0);
}

seal_value __seal_os_rename(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "rename";

  seal_value old, new;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_STRING, SEAL_STRING), &old, &new);

  return SEAL_VALUE_BOOL(rename(AS_STRING(old), AS_STRING(new)) == 0);
}

seal_value __seal_os_mkdir(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "mkdir";

  seal_value path, mode;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_STRING, SEAL_STRING), &path, &mode);

  int mode8 = strtol(AS_STRING(mode), NULL, 8);
  mode_t old_umask = umask(0);
  umask(old_umask);

  mode8 &= ~old_umask;

  return SEAL_VALUE_BOOL(mkdir(AS_STRING(path), mode8) == 0);
}

seal_value __seal_os_rmdir(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "rmdir";

  seal_value path;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &path);

  return SEAL_VALUE_BOOL(rmdir(AS_STRING(path)) == 0);
}

seal_value __seal_os_isfile(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "isfile";

  seal_value path;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &path);

  struct stat st;
  if (stat(AS_STRING(path), &st) != 0)
    return SEAL_VALUE_FALSE;

  return SEAL_VALUE_BOOL(S_ISREG(st.st_mode));
}

seal_value __seal_os_isdir(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "isdir";

  seal_value path;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_STRING), &path);

  struct stat st;
  if (stat(AS_STRING(path), &st) != 0)
    return SEAL_VALUE_FALSE;

  return SEAL_VALUE_BOOL(S_ISDIR(st.st_mode));
}

seal_value seal_init_mod()
{
  seal_value mod = {
    .type = SEAL_MOD,
    .as.mod = SEAL_CALLOC(1, sizeof(struct seal_module))
  };
  AS_MOD(mod)->name = MOD_NAME;
  AS_MOD(mod)->globals = SEAL_CALLOC(1, sizeof(hashmap_t));
  hashmap_init(AS_MOD(mod)->globals, 64);

  MOD_REGISTER_FUNC(mod, __seal_os_system,   "system",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_getenv,   "getenv",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_setenv,   "setenv",   3, false);
  MOD_REGISTER_FUNC(mod, __seal_os_unsetenv, "unsetenv", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_getcwd,   "getcwd",   0, false);
  MOD_REGISTER_FUNC(mod, __seal_os_chdir,    "chdir",    1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_readdir,  "readdir",  1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_remove,   "remove",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_rename,   "rename",   2, false);
  MOD_REGISTER_FUNC(mod, __seal_os_mkdir,    "mkdir",    2, false);
  MOD_REGISTER_FUNC(mod, __seal_os_rmdir,    "rmdir",    1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_isfile,   "isfile",   1, false);
  MOD_REGISTER_FUNC(mod, __seal_os_isdir,    "isdir",    1, false);

  /* global symbols */
  MOD_REGISTER_SYM(mod, "name", SEAL_VALUE_STRING_STATIC(
#ifdef __linux__
        "Linux"
#elif defined(_WIN32)
        "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
        "macOS"
#elif defined(__FreeBSD__)
        "FreeBSD"
#else
        "Unknown"
#endif
  ));
  MOD_REGISTER_SYM(mod, "arch", SEAL_VALUE_STRING_STATIC(
#ifdef __i386__
        "i386"
#elif defined(__x86_64__)
        "x86_64"
#elif defined(__arm__)
        "arm"
#elif defined(__aarch64__)
        "aarch64"
#endif
  ));

  return mod;
}
