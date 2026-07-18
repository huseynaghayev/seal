#include "seal.h"
#include "value.h"
#include "state.h"
#include <stdio.h> /* fopen, fclose */

#define FILE_METAMAP_NAME "FILE*"

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
    seal_setmetamap(S, FILE_METAMAP_NAME);
}

static void io_write(seal_state *S)
{
    int n = seal_gettop(S);
    struct seal_value *args = S->stack + S->sp - n;

    for (int i = 0; i < n; i++) {
        seal_print_val(args + i, false);
    }
    fflush(stdout);

    seal_pushnull(S);
}

static void file_close(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);
    fclose(file);
    seal_pushnull(S);
}

static void file_write(seal_state *S)
{
    seal_checkargc(S, 2);
    FILE *file = seal_checkuserdata(S, 0);
    const char *content = seal_checkstring(S, 1);
    fwrite(content, 1, strlen(content), file);
    seal_pushnull(S);
}

static void file_read(seal_state *S)
{
    seal_checkargcrange(S, 1, 2);
    FILE *file = seal_checkuserdata(S, 0);
    size_t fsize;
    if (seal_gettop(S) == 1) {
        fseek(file, 0, SEEK_END);
        fsize = ftell(file);
        fseek(file, 0, SEEK_SET);
    } else {
        fsize = seal_checkint(S, 1);
    }
    char *content = SEAL_MALLOC(fsize + 1);
    fread(content, 1, fsize, file);
    content[fsize] = '\0';
    seal_pushallocdstring(S, content);
}

static void file_readline(seal_state *S)
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

static void file_readlines(seal_state *S)
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
        if (c == '\n') {
            while ((c = fgetc(file)) == '\n');
            if (c != EOF && c != '\n')
                content[len++] = c;
        } else {
            content[len++] = c;
        }

        while ((c = fgetc(file)) != '\n' && c != EOF) {
            if (len + 1 >= cap) {
                cap *= 2;
                content = SEAL_REALLOC(content, cap);
            }
            content[len++] = c;
        }
        if (len > 0) {
            content[len] = '\0';
            seal_pushallocdstring(S, content);
            size++;
        } else {
            SEAL_FREE(content);
        }
    }
    seal_makelist(S, size);
}

static void file_flush(seal_state *S)
{
    seal_checkargc(S, 1);
    FILE *file = seal_checkuserdata(S, 0);
    fflush(file);
    seal_pushnull(S);
}

#define REG_IO(name) { #name, io_##name }

static const seal_reg iolib[] = {
    REG_IO(open),
    REG_IO(write),
    { NULL, NULL }
};

#define REG_FILE(name) { #name, file_##name }

static const seal_reg file_methods[] = {
    REG_FILE(close),
    REG_FILE(read),
    REG_FILE(readline),
    REG_FILE(readlines),
    REG_FILE(write),
    REG_FILE(flush),
    { NULL, NULL }
};

void sealopen_io(seal_state *S)
{
    seal_newmetamap(S, FILE_METAMAP_NAME);
    seal_regfields(S, file_methods);
    seal_movetop(S, -1);
    seal_newlib(S, iolib);

    /* stdin, stdout, stderr */
    seal_pushuserdata(S, stdin);
    seal_setmetamap(S, FILE_METAMAP_NAME);
    seal_setfield(S, -2, "stdin");

    seal_pushuserdata(S, stdout);
    seal_setmetamap(S, FILE_METAMAP_NAME);
    seal_setfield(S, -2, "stdout");

    seal_pushuserdata(S, stderr);
    seal_setmetamap(S, FILE_METAMAP_NAME);
    seal_setfield(S, -2, "stderr");

    seal_setglobal(S, "IO");
}
