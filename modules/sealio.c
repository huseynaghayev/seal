#include <seal.h>


static const char *MOD_NAME = "io";
static const char *FILE_NAME = "FILE*";


seal_value __seal_io_open(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "open";

  seal_value path, file_mode;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_STRING, SEAL_STRING), &path, &file_mode);

  FILE *fp = fopen(AS_STRING(path), AS_STRING(file_mode));

  return fp ? SEAL_VALUE_PTR(fp, FILE_NAME) : SEAL_VALUE_NULL;
}

seal_value __seal_io_close(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "close";

  seal_value file_ptr;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_PTR), &file_ptr, FILE_NAME);
  fclose(AS_PTR(file_ptr).ptr);

  return SEAL_VALUE_NULL;
}

seal_value __seal_io_read(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "read";

  seal_value file_ptr;
  SEAL_PARSE_ARGS(1, PARAM_TYPES(SEAL_PTR), &file_ptr, FILE_NAME);

  FILE *file = AS_PTR(file_ptr).ptr;

	if (!file) {
		return SEAL_VALUE_NULL;
	} else {
		fseek(file, 0, SEEK_END);
		unsigned len = ftell(file);
		char* content = SEAL_CALLOC(len + 1, sizeof(char));
		fseek(file, 0, SEEK_SET);
		if (content) {
			fread(content, sizeof(char), len, file);
		}
		content[len] = '\0';

		return SEAL_VALUE_STRING(content);
	}
}

seal_value __seal_io_write(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "write";

  seal_value file_ptr, to_write;
  SEAL_PARSE_ARGS(2, PARAM_TYPES(SEAL_PTR, SEAL_STRING), &file_ptr, FILE_NAME, &to_write);

  FILE *file = AS_PTR(file_ptr).ptr;

  fputs(AS_STRING(to_write), file);
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
  hashmap_init(AS_MOD(mod)->globals, 32);

  MOD_REGISTER_FUNC(mod, __seal_io_open,  "open",  2, false);
  MOD_REGISTER_FUNC(mod, __seal_io_close, "close", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_io_read,  "read",  1, false);
  MOD_REGISTER_FUNC(mod, __seal_io_write, "write", 2, false);

  return mod;
}
