#ifndef SEAL_VM_H
#define SEAL_VM_H

#include "sealconf.h"
#include "compiler.h"
#include "sealtypes.h"
#include "hashmap.h"

#define STACK_SIZE (0xFFFF + 1)
#define GLOBAL_SIZE (0xFF + 1)
#define FRAME_MAX (0xFF + 1)

typedef struct vm vm_t;

struct local_frame {
  svalue_t* locals;
  seal_byte* ip;
  seal_byte* bytecodes;
  struct line_info *linfo;
  int linfo_size;
  svalue_t *const_pool;
  seal_word *label_pool;
  hashmap_t *globals;
  const char *file_name;
};

struct vm {
 svalue_t* const_pool_ptr; /* pointer to pool for constant values */
 seal_word* label_ptr; /* pointer to array of labels */
 seal_byte*  bytecodes;  /* bytecode array (do not increment this) */ 
 //uint8_t*  ip;    /* instruction pointer */
 svalue_t* stack; /* stack */
 svalue_t* sp;    /* stack pointer */
 hashmap_t globals; /* hashmap for globals */
 //struct local_frame* lf; /* local frame for function calls */
};

void init_mod_cache();
svalue_t insert_mod_cache(const char*);

void init_vm(vm_t* vm, cout_t* cout);
void eval_vm(vm_t* vm, struct local_frame* lf);

static void print_stack(vm_t* vm)
{
  svalue_t* sp = vm->sp;
  while (sp > vm->stack) {
    svalue_t val = *(--sp);
    switch (val.type) {
      case SEAL_NULL:
        printf("null\n");
        break;
      case SEAL_INT:
        printf("%lld\n", val.as._int);
        break;
      case SEAL_FLOAT:
        printf("%f\n", val.as._float);
        break;
      case SEAL_STRING:
        printf("%s\n", val.as.string->val);
        break;
      case SEAL_BOOL:
        printf("%s\n", val.as._bool ? "true" : "false");
        break;
      case SEAL_FUNC:
        printf("function: %p\n", val.as.func.type == FUNC_BUILTIN ? (void*)val.as.func.as.builtin.cfunc : (void*)val.as.func.as.userdef.bytecode);
        break;
      default:
        printf("STACK TYPE UNRECOGNIZED: %d\n", val.type);
        break;
    }
  }
}

#endif /* SEAL_VM_H */
