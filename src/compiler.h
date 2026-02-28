#ifndef COMPILER_H
#define COMPILER_H


#include "sealconf.h"
#include "ast.h"
#include "value.h"


enum {
    /* TOP := Top of the stack */
    /* TOP - n := n steps down from TOP */
    /* IP  := Instruction pointer */
    /* POOL  := Constant pool */
    /* NEXT  := Next byte */
    /* 2NEXT := Next 2 bytes */
    OP_HALT,  /* halt the program */
    OP_PUSH,  /* TOP := POOL[NEXT] */
    OP_LPUSH, /* TOP := POOL[2NEXT] */
    OP_PUSHNULL,  /* TOP := null */
    OP_PUSHTRUE,  /* TOP := true */
    OP_PUSHFALSE, /* TOP := false */
    OP_PUSH8,  /* TOP := NEXT */
    OP_PUSH16, /* TOP := 2NEXT */
    OP_POP,    /* Pop TOP */
    OP_DUP,    /* Duplicate TOP */
    OP_COPY,   /* TOP := TOP - NEXT */
    OP_SWAP,   /* TOP := TOP - NEXT, TOP - NEXT := TOP */
    OP_JMP,    /* IP += (signed)2NEXT */
    OP_JTRUE,  /* If TOP != (null | false) then IP += (signed)2NEXT */
    OP_JFALSE, /* If TOP == (null | false) then IP += (signed)2NEXT */
    OP_JNULL,  /* If TOP == null then IP += (signed)2NEXT */
    OP_CALL,   /* Get NEXT size arguments and call function */
    OP_RETURN, /* Get back to previous call frame */
    /* binaries
     * 'b' is popped first, 'a' is second.
     * if there is only 'a', it means only a popped.
     */
    OP_ADD, /* PUSH(a + b)  */
    OP_SUB, /* PUSH(a - b)  */
    OP_MUL, /* PUSH(a * b)  */
    OP_DIV, /* PUSH(a / b)  */
    OP_MOD, /* PUSH(a % b)  */
    OP_AND, /* PUSH(a & b)  */
    OP_OR,  /* PUSH(a | b)  */
    OP_XOR, /* PUSH(a ^ b)  */
    OP_SHL, /* PUSH(a << b) */
    OP_SHR, /* PUSH(a >> b) */
    OP_GT,  /* PUSH(a > b)  */
    OP_GE,  /* PUSH(a >= b) */
    OP_LT,  /* PUSH(a < b)  */
    OP_LE,  /* PUSH(a <= b) */
    OP_EQ,  /* PUSH(a == b) */
    OP_NE,  /* PUSH(a != b) */
    /* unaries */
    OP_NOT,  /* PUSH( (! | not) a ) */
    OP_BNOT, /* PUSH( ~ a ) */
    OP_NEG,  /* PUSH( - a ) */
    /* symbols */
    /* without $ if a symbol is not found locally,
     * it will get searched as global.
     * With $, it is always global.
     */
    OP_GETGLOBAL, /* $name | name -- hashmap POOL[2NEXT] */
    OP_GETGLOBAL_SAFE, /* $name? | name? return null if not found, else itself */
    OP_SETGLOBAL, /* $name = val */
    OP_GETLOCAL,  /* name -- LOCALS[NEXT] */
    OP_SETLOCAL,  /* name = val */
    /* lists and maps */
    OP_NEWLIST,  /* TOP := new empty list */
    OP_MAKELIST, /* Push NEXT amount of elements to newly created empty list */
    OP_PUSHLIST, /* Push NEXT amount of elements to list and keep list on stack */
    OP_NEWMAP,   /* TOP := new empty map */
    OP_MAKEMAP,  /* Push NEXT amount of elements to newly created empty map */
    OP_PUSHMAP,  /* Push to map and keep map on stack */
    OP_GETFIELD, /* a.b */
    OP_GETFIELD_SAFE, /* a.b? */
    OP_SETFIELD, /* a.b = POP() */
    OP_GETINDEX, /* a[b] */
    OP_SETINDEX, /* a.b = POP() */
    /* other */
    OP_INCLUDE, /* include POOL[2NEXT] => module name */
};

typedef struct {
    int line;
    int offset;
} line_info;

struct chunk {
    seal_byte *code; /* instructions */
    int code_size;
    struct seal_value *pool; /* constant pool */
    int pool_size;
    line_info *li;
    int li_size;
    int local_size;
};

int get_line(struct chunk *c, int ip);

struct chunk compile(struct ast *a, struct seal_hashmap *h);

#if SEAL_DEBUG
void dump_chunk(struct chunk *c);
void dump_bytecode(struct chunk *c);
#endif


#endif /* COMPILER_H */
