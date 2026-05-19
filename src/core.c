#include "state.h"
#include "value.h"
#include <stdio.h>

typedef struct seal_value value;

static void print_val(value *v, int inside_obj)
{
    switch (v->type) {
    case SEAL_TNULL:
        printf("null");
        break;
    case SEAL_TBOOL:
        printf("%s", SEAL_AS_BOOL(*v) ? "true" : "false");
        break;
    case SEAL_TINT:
        printf("%lld", SEAL_AS_INT(*v));
        break;
    case SEAL_TFLOAT:
    {
        char buf[32];
        seal_format_float(SEAL_AS_FLOAT(*v), buf, sizeof(buf));
        printf("%s", buf);
        break;
    }
    case SEAL_TSTRING:
        if (inside_obj)
            putchar('\'');

        printf("%s", SEAL_AS_STRINGVAL(*v));

        if (inside_obj)
            putchar('\'');

        break;
    case SEAL_TLIST:
    {
        //printf("list: %p", (void*)SEAL_AS_LIST(*v));
        putchar('[');
        struct seal_list *l = SEAL_AS_LIST(*v);
        int len = l->len;
        for (int i = 0; i < len; i++) {
            print_val(&l->vals[i], true);
            if (i < len - 1) {
                putchar(',');
                putchar(' ');
            } else {
                break;
            }
        }
        putchar(']');
        break;
    }
    case SEAL_TMAP:
    {
        //printf("map: %p", (void*)SEAL_AS_MAP(*v));
        putchar('{');
        int printed = 0;
        int len = SEAL_AS_MAP(*v)->len;
        for (int i = 0; i < SEAL_AS_MAP(*v)->cap; i++) {
            struct h_entry e = SEAL_AS_MAP(*v)->entries[i];
            if (e.key) {
                printf("%s = ", e.key);
                print_val(&e.val, true);
                printed++;
                if (printed < len) {
                    putchar(',');
                    putchar(' ');
                } else {
                    break;
                }
            }
        }
        putchar('}');
        break;
    }
    case SEAL_TFUNCTION:
        printf("function: %p", (void*)SEAL_AS_FUNC(*v));
        break;
    }
}

static void core_print(seal_state *S)
{
    int n = seal_gettop(S);
    value *args = S->stack + S->sp - n;

    for (int i = 0; i < n; i++) {
        print_val(args + i, false);
        if (i < n - 1)
            putchar(' ');
    }
    putchar('\n');

    seal_pushnull(S);
}

static void core_read(seal_state *S)
{
    int n = seal_gettop(S);
    value *args = S->stack + S->sp - n;

    for (int i = 0; i < n; i++) {
        print_val(args + i, false);
        if (i < n - 1)
            putchar(' ');
    }

    char buf[512];
    fgets(buf, sizeof(buf), stdin);
    *strchr(buf, '\n') = '\0';
    seal_pushstring(S, buf);
}

static void core_exit(seal_state *S)
{
    seal_checkargc(S, 1);
    exit(seal_checkint(S, 0));
    seal_pushnull(S); /* Pointless, because program was already terminated.
                       * But keep it as a convention.
                       */
}

static void core_typeof(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushstringc(S, seal_gettypename(S, 0));
}

static void core_string(seal_state *S)
{
    char buf[64];
    seal_checkargc(S, 1);
    switch (seal_gettype(S, 0)) {
    case SEAL_TBOOL:
        seal_pushstringc(S, seal_tobool(S, 0) ? "true" : "false");
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
        seal_pushstringc(S, seal_gettypename(S, 0));
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
