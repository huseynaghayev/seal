#ifndef SEALCONF_H
#define SEALCONF_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define VERSION "0.1.0"

#define SEAL_MALLOC(size)       malloc(size)
#define SEAL_CALLOC(n, size)    calloc(n, size)
#define SEAL_REALLOC(ptr, size) realloc(ptr, size)
#define SEAL_FREE(ptr)          free(ptr)

#define ERR_LEN 256
#define LOCAL_MAX 255

typedef long long seal_int;
typedef double    seal_float;
typedef uint8_t   seal_byte;
typedef uint16_t  seal_word;

#endif /* SEALCONF_H */
