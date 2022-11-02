#ifndef ABC__ext__ER_h
#define ABC__ext__ER_h

#include "base/main/main.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include "base/io/ioAbc.h" // Io_ReadBlif

ABC_NAMESPACE_HEADER_START

// ER.cpp
extern void Sample_MC(Abc_Ntk_t * pNtk, int nKey);
extern void Sample_MC_Miter(Abc_Ntk_t * pNtk, int nKey);

ABC_NAMESPACE_HEADER_END
#endif