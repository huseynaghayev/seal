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

    /* TODO: FIX IT */
    static char buf[1024];
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
    seal_pushstring(S, seal_gettypename(S, 0));
}

void sealopen_core(seal_state *S)
{
    seal_register(S, core_print,  "print");
    seal_register(S, core_read,   "read");
    seal_register(S, core_exit,   "exit");
    seal_register(S, core_typeof, "typeof");
}
