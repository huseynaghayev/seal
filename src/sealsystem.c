#include "seal.h"
#include <stdio.h>
#include <time.h>

#if SEAL_DEBUG
#include "state.h"
#endif

#if defined(__linux__)
#include <unistd.h>
static const char *osname = "Linux";
#elif defined(_WIN32)
#include <windows.h>
static const char *osname = "Windows";
#elif defined(__APPLE__) && defined(__MACH__)
#include <unistd.h>
static const char *osname = "MacOS";
#else
static const char *osname = "Unknown";
#endif

#if defined(__x86_64__) || defined(_M_X64)
static const char *arch = "x86_64";
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
static const char *arch = "i386";
#elif defined(__aarch64__) || defined(_M_ARM64)
static const char *arch = "arm64";
#else
static const char *arch = "Unknown";
#endif

static void system_shell(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, system(seal_checkstring(S, 0)));
}

static void system_time(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushint(S, time(NULL));
}

static void system_clock(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushfloat(S, (seal_float)clock() / (seal_float)CLOCKS_PER_SEC);
}

static void system_date(seal_state *S)
{
    seal_checkargcrange(S, 0, 2);
    const char *fmt;
    time_t timer;
    struct tm *t;
    int n = seal_gettop(S);
    switch (n) {
    case 0:
        fmt = "%Y-%m-%d";
        time(&timer);
        t = localtime(&timer);
        break;
    case 1:
        fmt = seal_checkstring(S, 0);
        time(&timer);
        t = localtime(&timer);
        break;
    case 2:
        fmt = seal_checkstring(S, 0);
        timer = seal_checkint(S, 1);
        t = localtime(&timer);
        break;
    }
    char date[128];
    strftime(date, sizeof(date), fmt, t);
    seal_pushstring(S, date);
}

static void system_sleep(seal_state *S)
{
    seal_checkargc(S, 1);
#ifdef _POSIX_VERSION
    seal_pushint(S, usleep(seal_checkint(S, 0) * 1000));
#else
    Sleep(seal_checkint(S, 0));
    seal_pushnull(S);
#endif
}

static void system_getenv(seal_state *S)
{
    seal_checkargc(S, 1);
    const char *name = getenv(seal_checkstring(S, 0));
    if (name) {
        seal_pushstring(S, name);
    } else {
        seal_pushnull(S);
    }
}

static void system_setenv(seal_state *S)
{
    seal_checkargcrange(S, 2, 3);
    const char *name = seal_checkstring(S, 0);
    const char *val  = seal_checkstring(S, 1);
    bool overwrite = true;
    if (seal_gettop(S) > 2) {
        overwrite = seal_checkbool(S, 2);
    }
    bool status = setenv(name, val, overwrite) == 0;
    seal_pushbool(S, status);
}

static void system_unsetenv(seal_state *S)
{
    seal_checkargc(S, 1);
    const char *name = seal_checkstring(S, 0);
    int status = unsetenv(name) == 0;
    seal_pushbool(S, status);
}

static void system_exist(seal_state *S)
{
    seal_checkargc(S, 1);
    const char *name = seal_checkstring(S, 0);
    seal_pushbool(S, access(name, F_OK) == 0);
}

static void system_remove(seal_state *S)
{
    seal_checkargc(S, 1);
    const char *name = seal_checkstring(S, 0);
    seal_pushbool(S, remove(name) == 0);
}

static void system_rename(seal_state *S)
{
    seal_checkargc(S, 2);
    const char *old = seal_checkstring(S, 0);
    const char *new = seal_checkstring(S, 1);
    seal_pushbool(S, rename(old, new) == 0);
}

static void system_getcwd(seal_state *S)
{
    seal_checkargc(S, 0);
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        seal_pushnull(S);
    } else {
        seal_pushstring(S, cwd);
    }
}

static void system_chdir(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushbool(S, chdir(seal_checkstring(S, 0)) == 0);
}

#define REG(name) { #name, system_##name }

static const seal_reg syslib[] = {
    REG(shell),
    REG(time),
    REG(clock),
    REG(date),
    REG(sleep),
    REG(getenv),
    REG(setenv),
    REG(unsetenv),
    REG(exist),
    REG(remove),
    REG(rename),
    REG(getcwd),
    REG(chdir),
    { NULL, NULL }
};

void sealopen_system(seal_state *S)
{
    seal_newlib(S, syslib);
    seal_pushconststring(S, osname);
    seal_setfield(S, -2, "name");
    seal_pushconststring(S, arch);
    seal_setfield(S, -2, "arch");
    seal_pushconststring(S, SEAL_VERSION);
    seal_setfield(S, -2, "version");
#if SEAL_DEBUG
    seal_push(S, SEAL_VMAP(S->packages));
    seal_setfield(S, -2, "packages");
    seal_push(S, SEAL_VMAP(S->globals));
    seal_setfield(S, -2, "globals");
#endif
    seal_setglobal(S, "System");
}
