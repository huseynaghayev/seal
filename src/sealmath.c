#include "seal.h"
#include <math.h>
#include <time.h> /* for random */
#include <stdint.h> /* uintptr_t */
#include "state.h" /* random_state */

static void math_abs(seal_state *S)
{
    seal_checkargc(S, 1);
    if (seal_isint(S, 0)) {
        seal_int n = seal_toint(S, 0);
        seal_pushint(S, llabs(n));
    } else {
        seal_float n = seal_checkfloat(S, 0);
        seal_pushfloat(S, fabs(n));
    }
}

static void math_sqrt(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, sqrt(seal_checknumber(S, 0)));
}

static void math_pow(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_pushfloat(S, pow(seal_checknumber(S, 0), seal_checknumber(S, 1)));
}

static void math_cbrt(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, cbrt(seal_checknumber(S, 0)));
}

static void math_sin(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, sin(seal_checknumber(S, 0)));
}

static void math_cos(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, cos(seal_checknumber(S, 0)));
}

static void math_tan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, tan(seal_checknumber(S, 0)));
}

static void math_asin(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, asin(seal_checknumber(S, 0)));
}

static void math_acos(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, acos(seal_checknumber(S, 0)));
}

static void math_atan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, atan(seal_checknumber(S, 0)));
}

static void math_atan2(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_pushfloat(S, atan2(seal_checknumber(S, 0), seal_checknumber(S, 1)));
}

static void math_exp(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, exp(seal_checknumber(S, 0)));
}

static void math_log(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log(seal_checknumber(S, 0)));
}

static void math_log10(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log10(seal_checknumber(S, 0)));
}

static void math_log2(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log2(seal_checknumber(S, 0)));
}

static void math_floor(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)floor(seal_checknumber(S, 0)));
}

static void math_ceil(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)ceil(seal_checknumber(S, 0)));
}

static void math_round(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)round(seal_checknumber(S, 0)));
}

static void math_trunc(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)trunc(seal_checknumber(S, 0)));
}

static void math_min(seal_state *S)
{
    seal_checkargcmin(S, 2);
    int n = seal_gettop(S);

    double m = seal_checknumber(S, 0);
    int type = seal_isint(S, 0) ? SEAL_TINT : SEAL_TFLOAT;
    for (int i = 1; i < n; i++) {
        double cur = seal_checknumber(S, i);
        int cur_type = seal_isint(S, i) ? SEAL_TINT : SEAL_TFLOAT;

        if (cur < m) {
            type = cur_type;
            m = cur;
        }
    }

    if (type == SEAL_TINT) {
        seal_pushint(S, m);
    } else {
        seal_pushfloat(S, m);
    }
}

static void math_max(seal_state *S)
{
    seal_checkargcmin(S, 2);
    int n = seal_gettop(S);

    double m = seal_checknumber(S, 0);
    int type = seal_isint(S, 0) ? SEAL_TINT : SEAL_TFLOAT;
    for (int i = 1; i < n; i++) {
        double cur = seal_checknumber(S, i);
        int cur_type = seal_isint(S, i) ? SEAL_TINT : SEAL_TFLOAT;

        if (cur > m) {
            type = cur_type;
            m = cur;
        }
    }

    if (type == SEAL_TINT) {
        seal_pushint(S, m);
    } else {
        seal_pushfloat(S, m);
    }
}

static void math_isnan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isnan(seal_checknumber(S, 0)));
}

static void math_isinf(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isinf(seal_checknumber(S, 0)));
}

static void math_isfinite(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isfinite(seal_checknumber(S, 0)));
}

static void math_random(seal_state *S)
{
    seal_checkargcmax(S, 2);
    S->random_state = S->random_state * 1664525 + 1013904223;
    unsigned int r = S->random_state;
    int min, max;
    switch (seal_gettop(S)) {
    case 0:
        seal_pushfloat(S, (seal_float)r / (seal_float)0xFFFFFFFF);
        break;
    case 1:
        max = seal_checkint(S, 0);
        if (max <= 0) {
            seal_throw(S, "Math.random(max): \'max\' must be greater than zero");
        }
        seal_pushint(S, r % max);
        break;
    case 2:
        min = seal_checkint(S, 0);
        max = seal_checkint(S, 1);
        if (max <= min) {
            seal_throw(S, "Math.random(min, max): \'max\' must be greater than \'min\'");
        }
        seal_pushint(S, min + (signed int)(r % (unsigned int)(max - min)));
        break;
    }
}

#define REG(name) { #name, math_##name }

static const seal_reg mathlib[] = {
    REG(abs),
    REG(sqrt),
    REG(pow),
    REG(cbrt),
    REG(sin),
    REG(cos),
    REG(tan),
    REG(asin),
    REG(acos),
    REG(atan),
    REG(atan2),
    REG(exp),
    REG(log),
    REG(log10),
    REG(log2),
    REG(floor),
    REG(ceil),
    REG(round),
    REG(trunc),
    REG(min),
    REG(max),
    REG(isnan),
    REG(isinf),
    REG(isfinite),
    REG(random),
    { NULL, NULL }
};

void sealopen_math(seal_state *S)
{
    S->random_state = (unsigned int)time(NULL) ^ (uintptr_t)S;

    seal_newlib(S, mathlib);
    seal_pushfloat(S, M_PI);
    seal_setfield(S, -2, "PI");
    seal_pushfloat(S, M_E);
    seal_setfield(S, -2, "E");
    seal_pushfloat(S, INFINITY);
    seal_setfield(S, -2, "INF");
    seal_pushfloat(S, NAN);
    seal_setfield(S, -2, "NAN");
    seal_setglobal(S, "Math");
}
