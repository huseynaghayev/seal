#include "seal.h"
#include <string.h>
#include <ctype.h>

static void string_format(seal_state *S)
{
    //seal_checkargcvar(S, 1);
    //const char *fmt = seal_checkstring(S, 0);
    seal_pushnull(S);
}

static void string_lower(seal_state *S)
{
    seal_checkargc(S, 1);
    char *s = strdup(seal_checkstring(S, 0));
    int l = seal_getlength(S, 0);
    for (int i = 0; i < l; i++) {
        s[i] = tolower(s[i]);
    }
    seal_pushallocdstring(S, s);
}

static void string_upper(seal_state *S)
{
    seal_checkargc(S, 1);
    char *s = strdup(seal_checkstring(S, 0));
    int l = seal_getlength(S, 0);
    for (int i = 0; i < l; i++) {
        s[i] = toupper(s[i]);
    }
    seal_pushallocdstring(S, s);
}

#define is_func_creator(__name) \
static void string_##__name(seal_state *S) \
{ \
    seal_checkargc(S, 1); \
    const char *s = seal_checkstring(S, 0); \
    int l = seal_getlength(S, 0); \
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

static void string_startwith(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *str = seal_checkstring(S, 0);
    const char *start = seal_checkstring(S, 1);
    int lenstr = seal_getlength(S, 0);
    int lenstart = seal_getlength(S, 1);
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

static void string_endwith(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *str = seal_checkstring(S, 0);
    const char *end = seal_checkstring(S, 1);
    int lenstr = seal_getlength(S, 0);
    int lenend = seal_getlength(S, 1);
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

static void string_len(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_checktype(S, 0, SEAL_TSTRING);
    seal_pushint(S, seal_getlength(S, 0));
}

static void string_split(seal_state *S)
{
    seal_checkargcrange(S, 1, 2);
    const char *s = seal_checkstring(S, 0);
    const char *sep;
    if (seal_gettop(S) == 2)
        sep = seal_checkstring(S, 1);
    else
        sep = " ";

    int n = 0;
    int slen = seal_getlength(S, 0);
    int seplen = strlen(sep);
    int len = 0;
    int i = 0;
    if (seplen == 0) {
        while (*s) {
            seal_pushlstring(S, s, 1);
            s++;
        }
        seal_makelist(S, slen);
        return;
    }
    while (i < slen) {
        bool matched = true;
        if (i + seplen <= slen) {
            for (int j = 0; j < seplen; j++) {
                if (s[i + j] != sep[j]) {
                    matched = false;
                    break;
                }
            }
        }

        if (matched) {
            if (len != 0) {
                seal_pushlstring(S, s + (i - len), len);
                n++;
                len = 0;
            }
            i += seplen;
        } else {
            len++;
            i++;
        }
    }
    if (len != 0) {
        seal_pushlstring(S, s + (i - len), len);
        n++;
    }

    seal_makelist(S, n);
}

static void string_sub(seal_state *S)
{
    /* TODO: handle error and negative end cases */
    seal_checkargcrange(S, 2, 3);
    const char *s = seal_checkstring(S, 0);
    int start = seal_checkint(S, 1);
    int end;
    int len = seal_getlength(S, 0);
    if (seal_gettop(S) == 3) {
        end = seal_checkint(S, 2);
    } else {
        end = len;
    }
    if (start >= 0) {
        s += start;
    } else {
        s += len + start;
    }
    if (end <= 0) {
        end = len + end;
    }
    seal_pushlstring(S, s, end - start);
}

#define REG(name) { #name, string_##name }

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
    REG(split),
    REG(sub),
    { NULL, NULL }
};

void sealopen_string(seal_state *S)
{
    seal_newlib(S, strlib);
    seal_setglobal(S, "String");
}
