#include "state.h"
#include "value.h"
#include <stdio.h>

typedef struct seal_value value;

static void core_print(seal_state *S)
{
    int n = seal_gettop(S);
    value *args = S->stack + S->sp - n;

    for (int i = 0; i < n; i++) {
        seal_print_val(args + i, false);
        if (i < n - 1)
            putchar(' ');
    }
    putchar('\n');

    seal_pushnull(S);
}

static void core_read(seal_state *S)
{
    seal_checkargcmax(S, 1);


    if (seal_gettop(S) > 0) {
        seal_print_val(&seal_getstack(S, 0), false);
    }

    char buf[512];
    fgets(buf, sizeof(buf), stdin);
    *strchr(buf, '\n') = '\0';
    seal_pushstring(S, buf);
}

static void core_exit(seal_state *S)
{
    seal_checkargcmax(S, 1);
    int code = 0;
    if (seal_gettop(S) > 0) {
        code = seal_checkint(S, 0);
    }
    exit(code);
    seal_pushnull(S); /* Pointless, because program was already terminated.
                       * But keep it as a convention.
                       */
}

static void core_typeof(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushconststring(S, seal_gettypename(S, 0));
}

static void core_string(seal_state *S)
{
    char buf[64];
    seal_checkargc(S, 1);
    switch (seal_gettype(S, 0)) {
    case SEAL_TBOOL:
        seal_pushconststring(S, seal_tobool(S, 0) ? "true" : "false");
        break;
    case SEAL_TINT:
        snprintf(buf, sizeof(buf), "%lld", seal_toint(S, 0)); 
        seal_pushstring(S, buf);
        break;
    case SEAL_TFLOAT:
        seal_format_float(seal_tofloat(S, 0), buf, sizeof(buf));
        seal_pushstring(S, buf);
        break;
    case SEAL_TSTRING:
        seal_pushidx(S, 0);
        break;
    default:
        seal_pushconststring(S, seal_gettypename(S, 0));
        break;
    }
}

static void core_integer(seal_state *S)
{
    seal_checkargc(S, 1);
    switch (seal_gettype(S, 0)) {
    case SEAL_TBOOL:
        seal_pushint(S, seal_tobool(S, 0) ? 1 : 0);
        break;
    case SEAL_TINT:
        seal_pushidx(S, 0);
        break;
    case SEAL_TFLOAT:
        seal_pushint(S, (seal_int)seal_tofloat(S, 0));
        break;
    case SEAL_TSTRING:
        seal_pushint(S, (seal_int)atoll(seal_tostring(S, 0)));
        break;
    default:
        seal_throw(S, "cannot convert \'%s\' to \'integer\'", seal_gettypename(S, 0));
        break;
    }
}

static void core_float(seal_state *S)
{
    seal_checkargc(S, 1);
    switch (seal_gettype(S, 0)) {
    case SEAL_TBOOL:
        seal_pushfloat(S, seal_tobool(S, 0) ? 1.0 : 0.0);
        break;
    case SEAL_TINT:
        seal_pushfloat(S, (seal_float)seal_toint(S, 0));
        break;
    case SEAL_TFLOAT:
        seal_pushidx(S, 0);
        break;
    case SEAL_TSTRING:
        seal_pushfloat(S, (seal_float)atof(seal_tostring(S, 0)));
        break;
    default:
        seal_throw(S, "cannot convert \'%s\' to \'float\'", seal_gettypename(S, 0));
        break;
    }
}

void sealopen_core(seal_state *S)
{
    seal_register(S, core_print,   "print");
    seal_register(S, core_read,    "read");
    seal_register(S, core_exit,    "exit");
    seal_register(S, core_typeof,  "typeof");
    seal_register(S, core_string,  "string");
    seal_register(S, core_integer, "integer");
    seal_register(S, core_float,   "float");
}
