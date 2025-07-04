#include <seal.h>
#include <raylib.h>

#define color(r, g, b, a) ((Color) { r, g, b, a })

static const char *MOD_NAME = "raylib";
static const char *TEX_NAME = "Texture2D*";


/* Window-related functions */
seal_value __seal_raylib_init_window(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "init_window";

  seal_value w, h, title;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 3, PARAM_TYPES(SEAL_NUMBER, SEAL_NUMBER, SEAL_STRING), &w, &h, &title);

  InitWindow(AS_NUM(w), AS_NUM(h), AS_STRING(title));

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_close_window(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "close_window";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  CloseWindow();

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_window_should_close(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "window_should_close";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_BOOL(WindowShouldClose());
}

/* Drawing-related functions */
seal_value __seal_raylib_clear_background(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "clear_background";

  seal_value r, g, b;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 3, PARAM_TYPES(SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER), &r, &g, &b);

  ClearBackground((Color) { AS_NUM(r), AS_NUM(g), AS_NUM(b) });

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_begin_drawing(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "begin_drawing";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  BeginDrawing();

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_end_drawing(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "end_drawing";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  EndDrawing();

  return SEAL_VALUE_NULL;
}

/* Timing-related functions */
seal_value __seal_raylib_set_fps(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "set_fps";

  seal_value fps;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &fps);

  SetTargetFPS(AS_INT(fps));

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_delta_time(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "delta_time";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_FLOAT(GetFrameTime());
}
seal_value __seal_raylib_get_fps(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "get_fps";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_INT(GetFPS());
}

/* Input-related functions: keyboard */
seal_value __seal_raylib_is_key_pressed(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_key_pressed";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  return SEAL_VALUE_BOOL(IsKeyPressed(AS_INT(key)));
}
seal_value __seal_raylib_is_key_down(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_key_down";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  return SEAL_VALUE_BOOL(IsKeyDown(AS_INT(key)));
}
seal_value __seal_raylib_is_key_released(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_key_released";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  return SEAL_VALUE_BOOL(IsKeyReleased(AS_INT(key)));
}
seal_value __seal_raylib_is_key_up(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_key_up";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  return SEAL_VALUE_BOOL(IsKeyUp(AS_INT(key)));
}
seal_value __seal_raylib_set_exit_key(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "set_exit_key";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  SetExitKey(AS_INT(key));

  return SEAL_VALUE_NULL;
}

/* Input-related functions: mouse */
seal_value __seal_raylib_is_mouse_pressed(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_mouse_pressed";

  seal_value button;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &button);

  return SEAL_VALUE_BOOL(IsMouseButtonPressed(AS_INT(button)));
}
seal_value __seal_raylib_is_mouse_down(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_mouse_down";

  seal_value button;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &button);

  return SEAL_VALUE_BOOL(IsMouseButtonDown(AS_INT(button)));
}
seal_value __seal_raylib_is_mouse_released(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_mouse_released";

  seal_value button;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &button);

  return SEAL_VALUE_BOOL(IsMouseButtonReleased(AS_INT(button)));
}
seal_value __seal_raylib_is_mouse_up(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_mouse_up";

  seal_value button;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &button);

  return SEAL_VALUE_BOOL(IsMouseButtonUp(AS_INT(button)));
}
seal_value __seal_raylib_mouse_x(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "mouse_x";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_INT(GetMouseX());
}
seal_value __seal_raylib_mouse_y(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "mouse_y";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_INT(GetMouseY());
}

/* Basic shapes drawing functions */
seal_value __seal_raylib_draw_rectangle(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_rectangle";

  seal_value x, y, w, h, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  8,
                  PARAM_TYPES(SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER),
                  &x, &y, &w, &h, &r, &g, &b, &a);

  DrawRectangle(AS_NUM(x), AS_NUM(y), AS_NUM(w), AS_NUM(h), color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a)));

  return SEAL_VALUE_NULL;
}
/* Texture-related functions */
seal_value __seal_raylib_load_texture(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "load_texture";

  seal_value path;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_STRING), &path);

  Texture2D *tex_ptr = calloc(1, sizeof(Texture2D));
  *tex_ptr = LoadTexture(AS_STRING(path));

  return SEAL_VALUE_PTR(tex_ptr, TEX_NAME);
}
seal_value __seal_raylib_unload_texture(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "unload_texture";

  seal_value tex_ptr;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_PTR), &tex_ptr, TEX_NAME);

  UnloadTexture(*((Texture2D*)AS_PTR(tex_ptr).ptr));
  SEAL_FREE(AS_PTR(tex_ptr).ptr);

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_draw_texture(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_texture";

  seal_value tex_ptr, x, y, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  7,
                  PARAM_TYPES(SEAL_PTR, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER),
                  &tex_ptr, TEX_NAME, &x, &y, &r, &g, &b, &a);

  DrawTexture(*((Texture2D*)(AS_PTR(tex_ptr).ptr)), AS_NUM(x), AS_NUM(y), color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a)));

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
  hashmap_init(AS_MOD(mod)->globals, 256);

  MOD_REGISTER_FUNC(mod, __seal_raylib_init_window, "init_window", 3, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_close_window, "close_window", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_window_should_close, "window_should_close", 0, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_clear_background, "clear_background", 3, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_begin_drawing, "begin_drawing", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_end_drawing, "end_drawing", 0, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_set_fps, "set_fps", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_delta_time, "delta_time", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_get_fps, "get_fps", 0, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_pressed, "is_key_pressed", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_down, "is_key_down", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_released, "is_key_released", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_up, "is_key_up", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_set_exit_key, "set_exit_key", 1, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_pressed, "is_mouse_pressed", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_down, "is_mouse_down", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_released, "is_mouse_released", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_up, "is_mouse_up", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_mouse_x, "mouse_x", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_mouse_y, "mouse_y", 0, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_rectangle, "draw_rectangle", 8, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_load_texture, "load_texture", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_unload_texture, "unload_texture", 1, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_texture, "draw_texture", 7, false);

  return mod;
}
