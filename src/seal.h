/* This file is
 * for embedding Seal
 * into C applications
 * Huseyn Aghayev (c) 2026
 */

#ifndef SEAL_H
#define SEAL_H

#include "sealconf.h"

typedef struct seal_state seal_state;
typedef void (*seal_Cfunction) (seal_state *S);

#define SEAL_TNULL    0
#define SEAL_TBOOL    1
#define SEAL_TINT     2
#define SEAL_TFLOAT     3
#define SEAL_TSTRING    4
#define SEAL_TLIST   5
#define SEAL_TMAP    6
#define SEAL_TFUNCTION  7

#define SEAL_TFUNC_SEAL 0
#define SEAL_TFUNC_C  1

/* state */
SEAL_API seal_state *seal_state_new();
SEAL_API void seal_state_free(seal_state *S);

/* handling */
SEAL_API void seal_throw(seal_state *S, const char *msg, ...);
SEAL_API const char *seal_geterror(seal_state *S);

SEAL_API int seal_dostring(seal_state *S, const char *str);
SEAL_API int seal_dofile(seal_state *S, const char *file_name);
SEAL_API int seal_call(seal_state *S, int argc);
SEAL_API int seal_icall(seal_state *S, int argc);

/* stack */
SEAL_API int seal_gettop(seal_state *S);
SEAL_API void seal_movetop(seal_state *S, int offset);
/* checking */
SEAL_API void seal_checkargcrange(seal_state *S, int min, int max);
#define seal_checkargc(S, c) seal_checkargcrange(S, c, c)
#define seal_checkargcmin(S, c) seal_checkargcrange(S, c, -1)
#define seal_checkargcmax(S, c) seal_checkargcrange(S, -1, c)
SEAL_API int seal_gettype(seal_state *S, int i);
SEAL_API const char *seal_gettypename(seal_state *S, int i);
#define seal_isnull(S, i)   (seal_gettype(S, i) == SEAL_TNULL)
#define seal_isbool(S, i)   (seal_gettype(S, i) == SEAL_TBOOL)
#define seal_isint(S, i)    (seal_gettype(S, i) == SEAL_TINT)
#define seal_isfloat(S, i)  (seal_gettype(S, i) == SEAL_TFLOAT)
#define seal_isnumber(S, i) (seal_isint(S, i) || seal_isfloat(S, i))
#define seal_isstring(S, i) (seal_gettype(S, i) == SEAL_TSTRING)
#define seal_islist(S, i)   (seal_gettype(S, i) == SEAL_TLIST)
#define seal_ismap(S, i)    (seal_gettype(S, i) == SEAL_TMAP)
#define seal_isfunc(S, i)   (seal_gettype(S, i) == SEAL_TFUNCTION)

/* values */
SEAL_API seal_bool   seal_tobool(seal_state *S, int i);
SEAL_API seal_int    seal_toint(seal_state *S, int i);
SEAL_API seal_float  seal_tofloat(seal_state *S, int i);
SEAL_API seal_float  seal_tonumber(seal_state *S, int i);
SEAL_API const char *seal_tostring(seal_state *S, int i);

SEAL_API void        seal_checktype(seal_state *S, int i, int type);
SEAL_API seal_bool   seal_checkbool(seal_state *S, int i);
SEAL_API seal_int    seal_checkint(seal_state *S, int i);
SEAL_API seal_float  seal_checkfloat(seal_state *S, int i);
SEAL_API seal_float  seal_checknumber(seal_state *S, int i);
SEAL_API const char *seal_checkstring(seal_state *S, int i);

/* push */

SEAL_API void seal_pushidx(seal_state *S, int i);
SEAL_API void seal_pushnull(seal_state *S);
SEAL_API void seal_pushbool(seal_state *S, int b);
SEAL_API void seal_pushint(seal_state *S, seal_int n);
SEAL_API void seal_pushfloat(seal_state *S, seal_float f);
SEAL_API void seal_pushstringx(seal_state *S, const char *str, bool dup, bool is_const);
#define seal_pushstring(S, str)  seal_pushstringx(S, str, true, false)
#define seal_pushstringn(S, str) seal_pushstringx(S, str, false, false)
#define seal_pushstringc(S, str) seal_pushstringx(S, str, false, true)
SEAL_API void seal_pushCfunc(seal_state *S, seal_Cfunction f);
SEAL_API void seal_makelist(seal_state *S, int size);
#define seal_newlist(S) seal_makelist(S, 0)
SEAL_API void seal_makemap(seal_state *S, int size);
#define seal_newmap(S) seal_makemap(S, 0)

/* get */
/* return 0 if it exists, 1 if not found (nothing is pushed) */
SEAL_API int seal_getglobal(seal_state *S, const char *name); /* push value on top */
SEAL_API int seal_getindex(seal_state *S, int i);
/* return 0 if it exists
 * 1 if not found (null is pushed for now)
 * -1 if not map (null is pushed for now)
 */
SEAL_API int seal_getfield(seal_state *S, int map_i, const char *key);

/* set */
/* return 0 if it existed before, 1 if new */
SEAL_API int seal_setglobal(seal_state *S, const char *name); /* value is on top */
SEAL_API int seal_setindex(seal_state *S, int list_i, int i);
/* return 0 if it existed before
 * 1 if new
 * -1 if not map
 */
SEAL_API int seal_setfield(seal_state *S, int map_i, const char *key);

typedef struct {
    const char *name;
    seal_Cfunction f;
} seal_reg;

#define seal_register(S, f, name) ( \
    seal_pushCfunc(S, f), \
    seal_setglobal(S, name) \
)
SEAL_API void seal_newlib(seal_state *S, const seal_reg *reg);

#endif /* SEAL_H */
