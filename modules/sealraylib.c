#include <seal.h>
#include <raylib.h>

#define color(r, g, b, a) ((Color) { r, g, b, a })
#define vec2(x, y) ((Vector2) { x, y })
#define rec(x, y, w, h) ((Rectangle) { x, y, w, h })

static const char *MOD_NAME  = "raylib";
static const char *TEX_NAME  = "Texture2D*";
static const char *FONT_NAME = "Font*";


/* Window-related functions */
seal_value __seal_raylib_init_window(seal_byte argc, seal_value *argv)
{
  SetTraceLogLevel(LOG_NONE); /* do not print log */

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
seal_value __seal_raylib_is_key_pressed_repeat(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "is_key_pressed_repeat";

  seal_value key;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_INT), &key);

  return SEAL_VALUE_BOOL(IsKeyPressedRepeat(AS_INT(key)));
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
seal_value __seal_raylib_get_char_pressed(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "get_char_pressed";

  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 0, NULL);

  return SEAL_VALUE_INT(GetCharPressed());
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
seal_value __seal_raylib_draw_line(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_line";

  seal_value x, y, end_x, end_y, thick, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  9,
                  PARAM_TYPES(
                    SEAL_NUMBER, SEAL_NUMBER, /* start position */
                    SEAL_NUMBER, SEAL_NUMBER, /* end position */
                    SEAL_NUMBER, /* thickness */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* color (rgba) */
                  ),
                  &x, &y, &end_x, &end_y, &thick, &r, &g, &b, &a);

  DrawLineEx(
    vec2(AS_NUM(x), AS_NUM(y)),
    vec2(AS_NUM(end_x), AS_NUM(end_y)),
    AS_NUM(thick),
    color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a))
  );

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_draw_circle(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_circle";

  seal_value x, y, radius, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  7,
                  PARAM_TYPES(
                    SEAL_NUMBER, SEAL_NUMBER, /* start position */
                    SEAL_NUMBER, /* radius */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* color (rgba) */
                  ),
                  &x, &y, &radius, &r, &g, &b, &a);

  DrawCircleV(
    vec2(AS_NUM(x), AS_NUM(y)),
    AS_NUM(radius),
    color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a))
  );

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_draw_rectangle(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_rectangle";

  seal_value x, y, w, h, ox, oy, rot, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  11,
                  PARAM_TYPES(
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* rectangle */
                    SEAL_NUMBER, SEAL_NUMBER, /* origin */
                    SEAL_NUMBER, /* rotation */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* color (rgba) */
                  ),
                  &x, &y, &w, &h, &ox, &oy, &rot, &r, &g, &b, &a);

  DrawRectanglePro(
    rec(AS_NUM(x), AS_NUM(y), AS_NUM(w), AS_NUM(h)),
    vec2(AS_NUM(ox), AS_NUM(oy)),
    AS_NUM(rot),
    color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a))
  );

  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_draw_triangle(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_triangle";

  seal_value x1, y1, x2, y2, x3, y3, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  10,
                  PARAM_TYPES(
                    SEAL_NUMBER, SEAL_NUMBER,
                    SEAL_NUMBER, SEAL_NUMBER,
                    SEAL_NUMBER, SEAL_NUMBER,
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER,
                  ),
                  &x1, &y1, &x2, &y2, &x3, &y3, &r, &g, &b, &a);

  DrawTriangle(
    vec2(AS_NUM(x1), AS_NUM(y1)),
    vec2(AS_NUM(x2), AS_NUM(y2)),
    vec2(AS_NUM(x3), AS_NUM(y3)),
    color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a))
  );

  return SEAL_VALUE_NULL;
}

/* Texture-related functions */
seal_value __seal_raylib_load_texture(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "load_texture";

  seal_value path;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_STRING), &path);

  Texture2D *tex_ptr = SEAL_CALLOC(1, sizeof(Texture2D));
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

  seal_value tex_ptr, x, y, w, h, dx, dy, dw, dh, ox, oy, rot, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  16,
                  PARAM_TYPES(
                    SEAL_PTR, /* texture */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* source rec */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* dest rec */
                    SEAL_NUMBER, SEAL_NUMBER, /* origin vector */
                    SEAL_NUMBER, /* rotation */
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, /* color (rgba) */
                  ),
                  &tex_ptr, TEX_NAME, &x, &y, &w, &h, &dx, &dy, &dw, &dh, &ox, &oy, &rot, &r, &g, &b, &a);

  DrawTexturePro(
    *((Texture2D*)(AS_PTR(tex_ptr).ptr)),
    rec(AS_NUM(x), AS_NUM(y), AS_NUM(w), AS_NUM(h)),
    rec(AS_NUM(dx), AS_NUM(dy), AS_NUM(dw), AS_NUM(dh)),
    vec2(AS_NUM(ox), AS_NUM(oy)),
    AS_NUM(rot),
    color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a))
  );

  return SEAL_VALUE_NULL;
}

/* Text-related functions */
seal_value __seal_raylib_load_font(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "load_font";

  seal_value path, size;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 2, PARAM_TYPES(SEAL_STRING, SEAL_INT), &path, &size);

  Font *font_ptr = SEAL_CALLOC(1, sizeof(Font));
  *font_ptr = LoadFontEx(AS_STRING(path), AS_INT(size), NULL, 0);

  return SEAL_VALUE_PTR(font_ptr, FONT_NAME);
}
seal_value __seal_raylib_unload_font(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "unload_font";

  seal_value font_ptr;
  seal_parse_args(MOD_NAME, FUNC_NAME, argc, argv, 1, PARAM_TYPES(SEAL_PTR), &font_ptr, FONT_NAME);


  UnloadFont(*((Font*)AS_PTR(font_ptr).ptr));
  SEAL_FREE(AS_PTR(font_ptr).ptr);
  return SEAL_VALUE_NULL;
}
seal_value __seal_raylib_draw_text(seal_byte argc, seal_value *argv)
{
  static const char *FUNC_NAME = "draw_text";

  seal_value font_ptr, text, x, y, font_size, spacing, r, g, b, a;
  seal_parse_args(MOD_NAME,
                  FUNC_NAME,
                  argc,
                  argv,
                  10,
                  PARAM_TYPES(
                    SEAL_PTR,
                    SEAL_STRING,
                    SEAL_NUMBER, SEAL_NUMBER,
                    SEAL_NUMBER,
                    SEAL_NUMBER,
                    SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER, SEAL_NUMBER),
                    &font_ptr, FONT_NAME, &text, &x, &y, &font_size, &spacing, &r, &g, &b, &a
                  );

  DrawTextEx(*((Font*)AS_PTR(font_ptr).ptr), AS_STRING(text), vec2(AS_NUM(x), AS_NUM(y)), AS_NUM(font_size),
             AS_NUM(spacing), color(AS_NUM(r), AS_NUM(g), AS_NUM(b), AS_NUM(a)));

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
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_pressed_repeat, "is_key_pressed_repeat", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_down, "is_key_down", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_released, "is_key_released", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_key_up, "is_key_up", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_get_char_pressed, "get_char_pressed", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_set_exit_key, "set_exit_key", 1, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_pressed, "is_mouse_pressed", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_down, "is_mouse_down", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_released, "is_mouse_released", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_is_mouse_up, "is_mouse_up", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_mouse_x, "mouse_x", 0, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_mouse_y, "mouse_y", 0, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_line, "draw_line", 9, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_circle, "draw_circle", 7, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_rectangle, "draw_rectangle", 11, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_triangle, "draw_triangle", 10, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_load_texture, "load_texture", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_unload_texture, "unload_texture", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_texture, "draw_texture", 16, false);

  MOD_REGISTER_FUNC(mod, __seal_raylib_load_font, "load_font", 2, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_unload_font, "unload_font", 1, false);
  MOD_REGISTER_FUNC(mod, __seal_raylib_draw_text, "draw_text", 10, false);

  return mod;
}
