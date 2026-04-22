#include <seal.h>
#include <raylib.h>
#include <string.h>
#include <ctype.h>

// 0xFF2013FF
#define int2color(n) (Color) { \
    ((n) >> 24) & 0xFF, \
    ((n) >> 16) & 0xFF, \
    ((n) >> 8)  & 0xFF, \
    (n) & 0xFF \
}

#define __streq(s) (strcmp(__s, s) == 0)
#define __str2colorext(n, N) else if (__streq(#n)) { c = N; }

static Color str2color(const char *__s)
{
    Color c;
    if (__streq("lightgray")) {
        c = LIGHTGRAY;
    }
    __str2colorext(gray, GRAY)
    __str2colorext(darkgray, DARKGRAY)
    __str2colorext(yello, YELLOW)
    __str2colorext(gold, GOLD)
    __str2colorext(orange, ORANGE)
    __str2colorext(pink, PINK)
    __str2colorext(red, RED)
    __str2colorext(maroon, MAROON)
    __str2colorext(green, GREEN)
    __str2colorext(lime, LIME)
    __str2colorext(darkgreen, DARKGREEN)
    __str2colorext(skyblue, SKYBLUE)
    __str2colorext(blue, BLUE)
    __str2colorext(darkblue, DARKBLUE)
    __str2colorext(purple, PURPLE)
    __str2colorext(violet, VIOLET)
    __str2colorext(darkpurple, DARKPURPLE)
    __str2colorext(beige, BEIGE)
    __str2colorext(brown, BROWN)
    __str2colorext(darkbrown, DARKBROWN)
    __str2colorext(white, WHITE)
    __str2colorext(black, BLACK)
    __str2colorext(blank, BLANK)
    __str2colorext(magenta, MAGENTA)
    __str2colorext(raywhite, RAYWHITE)
    else {
        c = BLACK;
    }

    return c;
}

static Color handle_color(seal_state *S, int i) {
    Color c;
    if (seal_isint(S, i)) {
        c = int2color(seal_toint(S, i));
    } else if (seal_isstring(S, i)) {
        char buf[32];
        strncpy(buf, seal_tostring(S, i), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        char *ch = buf;
        while (*ch) {
            *ch = tolower(*ch);
            ch++;
        }
        c = str2color(buf);
    } else {
        seal_throw(
            S,
            "expected int or string for color, got \'%s\'",
            seal_gettypename(S, i)
        );
    }
    return c;
}

static void seal_InitWindow(seal_state *S)
{
    SetTraceLogLevel(LOG_NONE);
    seal_checkargc(S, 3);
    InitWindow(seal_checknumber(S, 0), seal_checknumber(S, 1), seal_checkstring(S, 2));
    seal_pushnull(S);
}

static void seal_CloseWindow(seal_state *S)
{
    seal_checkargc(S, 0);
    CloseWindow();
    seal_pushnull(S);
}

static void seal_WindowShouldClose(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushbool(S, WindowShouldClose());
}

static void seal_ClearBackground(seal_state *S)
{
    seal_checkargc(S, 1);
    ClearBackground(handle_color(S, 0));
    seal_pushnull(S);
}

static void seal_BeginDrawing(seal_state *S)
{
    seal_checkargc(S, 0);
    BeginDrawing();
    seal_pushnull(S);
}

static void seal_EndDrawing(seal_state *S)
{
    seal_checkargc(S, 0);
    EndDrawing();
    seal_pushnull(S);
}

static void seal_SetTargetFPS(seal_state *S)
{
    seal_checkargc(S, 1);
    SetTargetFPS(seal_checkint(S, 0));
    seal_pushnull(S);
}

static void seal_GetFrameTime(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushfloat(S, GetFrameTime());
}

static void seal_GetFPS(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushint(S, GetFPS());
}

#define __keystreq(s) (strcmp(keyname, s) == 0)
#define __mapkey(s, N) else if (__keystreq(s)) { keycode = N; }
#define __eventstreq(s) (strcmp(e, s) == 0)
static void seal_KeyEvent(seal_state *S)
{
    seal_checkargc(S, 2);

    const char *keyname = seal_checkstring(S, 0);
    const char *e = seal_checkstring(S, 1);
    KeyboardKey keycode;

    if (__keystreq("up")) {
        keycode = KEY_UP;
    }
    __mapkey("down", KEY_DOWN)
    __mapkey("left", KEY_LEFT)
    __mapkey("right", KEY_RIGHT)
    __mapkey("space", KEY_SPACE)
    __mapkey("escape", KEY_ESCAPE)
    __mapkey("enter", KEY_ENTER)
    __mapkey("tab", KEY_TAB)
    __mapkey("backspace", KEY_BACKSPACE)
    else if (strlen(keyname) == 1) {
        char c = *keyname;
        if (c >= '0' && c <= '9') {
            keycode = c;
        } else if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z') {
            keycode = toupper(c);
        }
    } else {
        seal_pushnull(S);
        return;
    }

    bool result;
    if (__eventstreq("down")) {
        result = IsKeyDown(keycode);
    } else if (__eventstreq("up")) {
        result = IsKeyUp(keycode);
    } else if (__eventstreq("pressed")) {
        result = IsKeyPressed(keycode);
    } else if (__eventstreq("released")) {
        result = IsKeyReleased(keycode);
    } else {
        seal_pushnull(S);
        return;
    }

    seal_pushbool(S, result);
}

static void seal_GetMouseX(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushint(S, GetMouseX());
}

static void seal_GetMouseY(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushint(S, GetMouseY());
}

static void seal_DrawRectangle(seal_state *S)
{
    seal_checkargc(S, 5);
    DrawRectangle(
        seal_checknumber(S, 0),
        seal_checknumber(S, 1),
        seal_checknumber(S, 2),
        seal_checknumber(S, 3),
        handle_color(S, 4)
    );
}

#define REG(name) { #name, seal_##name },

static const seal_reg rayliblib[] = {
    REG(InitWindow)
    REG(CloseWindow)
    REG(WindowShouldClose)
    REG(ClearBackground)
    REG(BeginDrawing)
    REG(EndDrawing)
    REG(SetTargetFPS)
    REG(GetFrameTime)
    REG(GetFPS)
    REG(KeyEvent)
    REG(GetMouseX)
    REG(GetMouseY)
    REG(DrawRectangle)
    { NULL, NULL }
};

SEAL_API void sealopen_raylib(seal_state *S)
{
    seal_newlib(S, rayliblib);
    seal_setglobal(S, "Raylib");
}
