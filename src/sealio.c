#include "seal.h"
#include "value.h"
#include "state.h"
#include <stdio.h> /* fopen, fclose */

#define REG(name) { #name, io_##name }

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
    char *content = SEAL_MALLOC(fsize);
    fread(content, 1, fsize, file);
    seal_pushallocdstring(S, 0);
}

#define REG(name) { #name, io_##name }

static const seal_reg iolib[] = {
    REG(open),
    REG(close),
    REG(write),
    REG(read),
    { NULL, NULL }
};

void sealopen_io(seal_state *S)
{
    seal_newlib(S, iolib);
    seal_setglobal(S, "IO");
}
