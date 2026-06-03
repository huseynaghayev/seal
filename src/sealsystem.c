#include "seal.h"
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

static void system_sleep(seal_state *S)
{
    seal_checkargc(S, 1);
    seal_pushint(S, usleep(seal_checkint(S, 0) * 1000));
}

#define REG(name) { #name, system_##name }

static const seal_reg syslib[] = {
    REG(shell),
    REG(time),
    REG(sleep),
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
