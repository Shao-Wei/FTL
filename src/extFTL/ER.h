#ifndef ABC__ext__ER_h
#define ABC__ext__ER_h

#include "base/main/main.h"
#include "base/abc/abc.h"  
// Set ABC_USE_LIBGMP to 1 during makefile to set proper linkage against GMP lib.
#include <gmp.h> // mpz_t

ABC_NAMESPACE_HEADER_START

typedef struct ER_Man_t_ ER_Man_t;

struct ER_Man_t_
{
    Vec_Ptr_t * vCount;
    mpz_t mean;
    mpz_t stdev;
};

// ERMan.c
extern ER_Man_t *      ER_ManStart();
extern void            ER_ManStop( ER_Man_t * p );

extern void            ER_CountPush( ER_Man_t * p, char * c);
extern void            ER_GetStats( ER_Man_t * p );
extern void            ER_PrintStats ( ER_Man_t * p );

extern void            ER_mpz_test(); // Test unit

// ER.cpp
extern void Sample_MC_Miter(Abc_Ntk_t * pNtk, int nKey, int fVerbose);

ABC_NAMESPACE_HEADER_END
#endif