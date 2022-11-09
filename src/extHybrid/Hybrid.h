#ifndef ABC__ext__FTL_HYBRID_h
#define ABC__ext__FTL_HYBRID_h

#include "base/main/main.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include "base/io/ioAbc.h" // Io_ReadBlif

ABC_NAMESPACE_HEADER_START

// Hybrid.cpp
extern Abc_Ntk_t * FTL_Hybrid(Abc_Ntk_t * pNtk);

ABC_NAMESPACE_HEADER_END
#endif