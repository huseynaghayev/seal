#include "core.h"
#include "value.h"
#include <stdio.h>


typedef struct seal_value value;


static void print_val(value *v)
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
        printf("%g", SEAL_AS_FLOAT(*v));
        break;
    case SEAL_TSTRING:
        printf("%s", SEAL_AS_STRINGVAL(*v));
        break;
    case SEAL_TLIST:
        printf("list: %p", (void*)SEAL_AS_LIST(*v));
        break;
    case SEAL_TMAP:
        printf("map: %p", (void*)SEAL_AS_MAP(*v));
        break;
    case SEAL_TFUNCTION:
        printf("function: %p", (void*)SEAL_AS_FUNC(*v));
        break;
    }
}

static void core_print(seal_state *S)
{
    int n = seal_gettop(S);
    value *args = S->stack - n;

    for (int i = 0; i < n; i++) {
        print_val(args + i);
        if (i < n - 1)
            putchar(' ');
    }
    putchar('\n');

    seal_pushnull(S);
}

void seal_core_init(seal_state *S)
{
    seal_pushCfunc(S, core_print);
    seal_setglobal(S, "print");
}
