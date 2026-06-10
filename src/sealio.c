#include "seal.h"
#include "value.h"
#include "state.h"
#include <stdio.h> /* fopen, fclose */

static void io_open(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *file_name = seal_checkstring(S, 0);
    const char *mode = seal_checkstring(S, 1);

    FILE *file = fopen(file_name, mode);
    if (!file) {
        seal_pushnull(S);
        return;
    }

    seal_pushuserdata(S, file);
}

static void io_close(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);
    fclose(file);
    seal_pushnull(S);
}

static void io_write(seal_state *S)
{
    seal_checkargc(S, 2);
    FILE *file = seal_checkuserdata(S, 0);
    const char *content = seal_checkstring(S, 1);
    fwrite(content, 1, strlen(content), file);
    seal_pushnull(S);
}

static void io_read(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);
    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *content = SEAL_MALLOC(fsize + 1);
    fread(content, 1, fsize, file);
    content[fsize] = '\0';
    seal_pushallocdstring(S, content);
}

static void io_readline(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);

    int c;
    if ((c = fgetc(file)) == EOF) {
        seal_pushnull(S);
        return;
    }

    int len = 0;
    int cap = 32;
    char *content = SEAL_MALLOC(cap);
    if (c != '\n')
        content[len++] = c;

    while ((c = fgetc(file)) != '\n' && c != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            content = SEAL_REALLOC(content, cap);
        }
        content[len++] = c;
    }
    content[len] = '\0';
    seal_pushallocdstring(S, content);
}

static void io_readlines(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);
    int size = 0;
    while (1) {
        int c;
        if ((c = fgetc(file)) == EOF) {
            break;
        }

        int len = 0;
        int cap = 32;
        char *content = SEAL_MALLOC(cap);
        if (c != '\n')
            content[len++] = c;

        while ((c = fgetc(file)) != '\n' && c != EOF) {
            if (len + 1 >= cap) {
                cap *= 2;
                content = SEAL_REALLOC(content, cap);
            }
            content[len++] = c;
        }
        content[len] = '\0';
        seal_pushallocdstring(S, content);
        size++;
    }
    seal_makelist(S, size);
}

static void io_printraw(seal_state *S)
{
    int n = seal_gettop(S);
    struct seal_value *args = S->stack + S->sp - n;

    for (int i = 0; i < n; i++) {
        seal_print_val(args + i, false);
    }
    fflush(stdout);

    seal_pushnull(S);
}

#define REG(name) { #name, io_##name }

static const seal_reg iolib[] = {
    REG(open),
    REG(close),
    REG(write),
    REG(read),
    REG(readline),
    REG(readlines),
    REG(printraw),
    { NULL, NULL }
};

void sealopen_io(seal_state *S)
{
    seal_newlib(S, iolib);
    seal_setglobal(S, "IO");
}
