#ifndef SEAL_BYTECODE_H
#define SEAL_BYTECODE_H

#include "sealconf.h"

enum {
  /* essential */
  OP_HALT       ,
  OP_PUSH_INT   ,
  OP_PUSH_NULL  ,
  OP_PUSH_TRUE  ,
  OP_PUSH_FALSE ,
  OP_PUSH_CONST ,
  OP_POP        ,
  OP_DUP        ,
  OP_COPY       ,
  OP_SWAP       ,
  OP_JUMP       ,
  OP_JTRUE      ,
  OP_JFALSE     ,
  OP_CALL       ,
  /* variable */
  OP_GET_GLOBAL ,
  OP_SET_GLOBAL ,
  OP_GET_LOCAL  ,
  OP_SET_LOCAL  ,
  /* arithmetic */
  OP_ADD        ,
  OP_SUB        ,
  OP_MUL        ,
  OP_DIV        ,
  OP_MOD        ,
  /* bitwise binary */
  OP_AND        ,
  OP_OR         ,
  OP_XOR        ,
  OP_SHL        ,
  OP_SHR        ,
  /* comparison */
  OP_GT         ,
  OP_GE         ,
  OP_LT         ,
  OP_LE         ,
  /* equality */
  OP_EQ         ,
  OP_NE         ,
  /* unary */
  OP_NOT        ,
  OP_NEG        ,
  OP_TYPOF      ,
  OP_BNOT       ,
  /* list */
  OP_GEN_LIST   ,
  /* iterable */
  OP_GET_FIELD  ,
  OP_SET_FIELD  ,
  /* membership */
  OP_IN         ,
  /* map */
  OP_GEN_MAP    ,
  /* include */
  OP_INCLUDE    ,
  /* for loop */
  OP_FOR_PREP,
  OP_FOR_NEXT,
  OP_FOR_STOP
};

#define PRINT_BYTE(bytecodes, size) for(int i = 0; i < size; i++) { \
    printf("%d ", bytecodes[i]); \
    if (i == size - 1) printf("\n"); \
  }

static inline const char* op_name(int op)
{
  switch (op) {
  /* essential */
  case OP_HALT      :  return "OP_HALT";
  case OP_PUSH_CONST:  return "OP_PUSH_CONST";
  case OP_PUSH_INT  :  return "OP_PUSH_INT";
  case OP_PUSH_NULL :  return "OP_PUSH_NULL";
  case OP_PUSH_TRUE :  return "OP_PUSH_TRUE";
  case OP_PUSH_FALSE:  return "OP_PUSH_FALSE";
  case OP_POP       :  return "OP_POP";
  case OP_DUP       :  return "OP_DUP";
  case OP_COPY      :  return "OP_COPY";
  case OP_SWAP      :  return "OP_SWAP";
  case OP_JUMP      :  return "OP_JUMP";
  case OP_JTRUE     :  return "OP_JTRUE";
  case OP_JFALSE    :  return "OP_JFALSE";
  case OP_CALL      :  return "OP_CALL";
  /* variable */
  case OP_GET_GLOBAL:  return "OP_GET_GLOBAL";
  case OP_SET_GLOBAL:  return "OP_SET_GLOBAL";
  case OP_GET_LOCAL :  return "OP_GET_LOCAL";
  case OP_SET_LOCAL :  return "OP_SET_LOCAL";
  /* arithmetic */
  case OP_ADD       :  return "OP_ADD";
  case OP_SUB       :  return "OP_SUB";
  case OP_MUL       :  return "OP_MUL";
  case OP_DIV       :  return "OP_DIV";
  case OP_MOD       :  return "OP_MOD";
  /* bitwise */
  case OP_AND       :  return "OP_AND";
  case OP_OR        :  return "OP_OR";
  case OP_XOR       :  return "OP_XOR";
  case OP_SHL       :  return "OP_SHL";
  case OP_SHR       :  return "OP_SHR";
  case OP_BNOT      :  return "OP_BNOT";
  /* comparison */
  case OP_EQ        :  return "OP_EQ";
  case OP_NE        :  return "OP_NE";
  case OP_GT        :  return "OP_GT";
  case OP_GE        :  return "OP_GE";
  case OP_LT        :  return "OP_LT";
  case OP_LE        :  return "OP_LE";
  /* logical */
  case OP_NOT       :  return "OP_NOT";
  /* typeof */
  case OP_TYPOF     :  return "OP_TYPOF";
  case OP_GEN_LIST  :  return "OP_GEN_LIST";
  case OP_GET_FIELD :  return "OP_GET_FIELD";
  case OP_SET_FIELD :  return "OP_SET_FIELD";
  case OP_GEN_MAP   :  return "OP_GEN_MAP";
  case OP_INCLUDE   :  return "OP_INCLUDE";
  case OP_FOR_PREP  :  return "OP_FOR_PREP";
  case OP_FOR_NEXT  :  return "OP_FOR_NEXT";
  case OP_FOR_STOP  :  return "OP_FOR_STOP";
  default           :  return "OP NOT RECOGNIZED";
  }
}

static inline void print_op(seal_byte* bytes, size_t byte_size, seal_word* labels, size_t label_size)
{
  for (int i = 0; i < byte_size;) {
    for (int j = 0; j < label_size; j++) {
      if (i == labels[j]) {
        printf("LABEL%d: %d\n", j, labels[j]);
      }
    }
    seal_byte op = bytes[i++];
    printf("%s ", op_name(op)); 
    switch (op) { /* check if opcode requires byte(s) */
    case OP_PUSH_CONST: case OP_PUSH_INT: {
      seal_byte left  = bytes[i++];
      seal_byte right = bytes[i++];
      seal_word idx  = (left << 8) | right;
      printf("%d", idx);
    }
    break;
    case OP_JUMP: case OP_JFALSE: case OP_JTRUE: case OP_GET_GLOBAL: case OP_SET_GLOBAL: case OP_FOR_PREP: {
      seal_byte left  = bytes[i++];
      seal_byte right = bytes[i++];
      seal_word idx  = (left << 8) | right;
      printf("%d", idx);
      break;
    }
    case OP_GET_LOCAL: case OP_SET_LOCAL: case OP_GEN_LIST: case OP_GEN_MAP: case OP_SWAP: case OP_COPY: {
      seal_byte slot = bytes[i++];
      printf("%d", slot);
      break;
    }
    case OP_CALL:
      printf("%d", bytes[i++]);   
      break;
    case OP_FOR_NEXT:
      printf("%d, ", bytes[i++]);
      seal_byte left  = bytes[i++];
      seal_byte right = bytes[i++];
      seal_word idx  = (left << 8) | right;
      printf("%d", idx);
    }
    printf("\n");
  }
}

#endif /* SEAL_BYTECODE_H */
