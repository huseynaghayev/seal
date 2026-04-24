#ifndef LIBS_H
#define LIBS_H

#include "state.h"

SEAL_API void sealopen_core(seal_state *S);
SEAL_API void sealopen_math(seal_state *S);
SEAL_API void sealopen_system(seal_state *S);

#endif /* LIBS_H */
