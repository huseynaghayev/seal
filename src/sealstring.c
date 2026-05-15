#include "seal.h"
#include <string.h>
#include <ctype.h>

static void str_format(seal_state *S)
{
    //seal_checkargcvar(S, 1);
    //const char *fmt = seal_checkstring(S, 0);
    seal_pushnull(S);
}

static void str_lower(seal_state *S)
{
    seal_checkargc(S, 1);
    char *s = strdup(seal_checkstring(S, 0));
    int l = strlen(s);
    for (int i = 0; i < l; i++) {
        s[i] = tolower(s[i]);
    }
    seal_pushstring(S, s);
}

static void str_upper(seal_state *S)
{
    seal_checkargc(S, 1);
    char *s = strdup(seal_checkstring(S, 0));
    int l = strlen(s);
    for (int i = 0; i < l; i++) {
        s[i] = toupper(s[i]);
    }
    seal_pushstring(S, s);
}

#define is_func_creator(__name) \
static void str_##__name(seal_state *S) \
{ \
    seal_checkargc(S, 1); \
    const char *s = seal_checkstring(S, 0); \
    int l = strlen(s); \
    if (l == 0) { \
        seal_pushbool(S, false); \
        return; \
    } \
    bool result = true; \
    for (int i = 0; i < l; i++) { \
        if (!__name(s[i])) { \
            result = false; \
            break; \
        } \
    } \
    seal_pushbool(S, result); \
}

is_func_creator(isalnum)
is_func_creator(isalpha)
is_func_creator(isdigit)
is_func_creator(islower)
is_func_creator(isupper)

static void str_startwith(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *str = seal_checkstring(S, 0);
    const char *start = seal_checkstring(S, 1);
    int lenstr = strlen(str);
    int lenstart = strlen(start);
    if (lenstart > lenstr) {
        seal_pushbool(S, false);
        return;
    }
    bool result = true;

    while (*start) {
        if (*str++ != *start++) {
            result = false;
            break;
        }
    }
    seal_pushbool(S, result);
}

static void str_endwith(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *str = seal_checkstring(S, 0);
    const char *end = seal_checkstring(S, 1);
    int lenstr = strlen(str);
    int lenend = strlen(end);
    if (lenend > lenstr) {
        seal_pushbool(S, false);
        return;
    }
    bool result = true;

    str += lenstr - lenend;
    while (*end) {
        if (*str++ != *end++) {
            result = false;
            break;
        }
    }
    seal_pushbool(S, result);
}

static void str_len(seal_state *S)
{
    seal_checkargc(S, 1);
    const char *s = seal_checkstring(S, 0);
    seal_pushint(S, strlen(s));
}

#define REG(name) { #name, str_##name }

static const seal_reg strlib[] = {
    REG(lower),
    REG(upper),
    REG(isalnum),    
    REG(isalpha),
    REG(isdigit),
    REG(islower),
    REG(isupper),
    REG(startwith),
    REG(endwith),
    REG(len),
    { NULL, NULL }
};

void sealopen_string(seal_state *S)
{
    seal_newlib(S, strlib);
    seal_setglobal(S, "String");
}
