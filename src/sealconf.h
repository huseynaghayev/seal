#ifndef SEAL_CONF_H
#define SEAL_CONF_H


#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>


/* this macro determines
 * size of integer and float
 * (either 32 or 64 bits)
 */
#define SEAL_32BITS 0

/* semantic versioning of Seal */
#define SEAL_VERSION_MAJOR "0"
#define SEAL_VERSION_MINOR "1"
#define SEAL_VERSION_PATCH "1"

/* full version */
#define SEAL_VERSION SEAL_VERSION_MAJOR "." SEAL_VERSION_MINOR "." SEAL_VERSION_PATCH


#define SEAL_MALLOC(size)    malloc(size)
#define SEAL_CALLOC(n, size) calloc(n, size)
#define SEAL_REALLOC(ptr, size) realloc(ptr, size)
#define SEAL_FREE(ptr) free(ptr)



#if SEAL_32BITS
    #define SEAL_INTEGER int
    #define SEAL_FLOAT   float
#else
    #define SEAL_INTEGER long long
    #define SEAL_FLOAT   double
#endif

#define SEAL_BOOL bool

#define SEAL_BYTE unsigned char


typedef SEAL_BYTE    seal_byte;
typedef SEAL_INTEGER seal_int;
typedef SEAL_FLOAT   seal_float;
typedef SEAL_BOOL    seal_bool;

#define USE_GNU_READL 0

#define HASHMAP_LOAD_FACTOR 0.7f


#endif /* SEAL_CONF_H */
