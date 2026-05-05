#ifndef VM_H
#define VM_H

#include "state.h"

int eval(seal_state *S, int till);

#if SEAL_DEBUG
void print_stack(seal_state *S);
#endif

#endif
