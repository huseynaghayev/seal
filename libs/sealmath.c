#include <seal.h>
#include <math.h>

static void seal_abs(seal_state *S)
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

static void seal_sqrt(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, sqrt(seal_checknumber(S, 0)));
}

static void seal_pow(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_pushfloat(S, pow(seal_checknumber(S, 0), seal_checknumber(S, 1)));
}

static void seal_cbrt(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, cbrt(seal_checknumber(S, 0)));
}

static void seal_sin(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, sin(seal_checknumber(S, 0)));
}

static void seal_cos(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, cos(seal_checknumber(S, 0)));
}

static void seal_tan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, tan(seal_checknumber(S, 0)));
}

static void seal_asin(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, asin(seal_checknumber(S, 0)));
}

static void seal_acos(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, acos(seal_checknumber(S, 0)));
}

static void seal_atan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, atan(seal_checknumber(S, 0)));
}

static void seal_atan2(seal_state *S)
{
    seal_checkargc(S, 2);
    seal_pushfloat(S, atan2(seal_checknumber(S, 0), seal_checknumber(S, 1)));
}

static void seal_exp(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, exp(seal_checknumber(S, 0)));
}

static void seal_log(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log(seal_checknumber(S, 0)));
}

static void seal_log10(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log10(seal_checknumber(S, 0)));
}

static void seal_log2(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushfloat(S, log2(seal_checknumber(S, 0)));
}

static void seal_floor(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)floor(seal_checknumber(S, 0)));
}

static void seal_ceil(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)ceil(seal_checknumber(S, 0)));
}

static void seal_round(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)round(seal_checknumber(S, 0)));
}

static void seal_trunc(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, (seal_int)trunc(seal_checknumber(S, 0)));
}

static void seal_min(seal_state *S)
{
    seal_checkargcvar(S, 2);
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

static void seal_max(seal_state *S)
{
    seal_checkargcvar(S, 2);
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

static void seal_isnan(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isnan(seal_checknumber(S, 0)));
}

static void seal_isinf(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isinf(seal_checknumber(S, 0)));
}

static void seal_isfinite(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, isfinite(seal_checknumber(S, 0)));
}

#define REG(name) { #name, seal_##name }

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
    { NULL, NULL }
};

SEAL_API void sealopen_math(seal_state *S)
{
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
