#ifndef ABC__ext__ER_h
#define ABC__ext__ER_h

#include "base/main/main.h"
#include "base/abc/abc.h"  
/* 
Notes on GMP library
- ABC_USE_LIBGMP is set to 1 in makefile by default to set proper linkage (-lgmp) against GMP lib.
- Create array of mpz_t by mem alloc size * sizeof(mpz_t).
- Setting array of mpz_t * (either Vec_Ptr_t * or mpz_t **) causes error since type mpz_t itself is an array.
*/
#include <gmp.h> // mpz_t

ABC_NAMESPACE_HEADER_START

typedef struct ER_Man_t_ ER_Man_t;

struct ER_Man_t_
{
    mpz_t * vCount; 
    int nPi;
    int nKey;
    int vCountSize;

    // Stats
    mpz_t mean;
    mpz_t stdev;
};

// ERMan.c --------------------
extern ER_Man_t *      ER_ManStart(int nPi, int nKey);
extern void            ER_ManStop( ER_Man_t * p );

extern void            ER_AddCount( ER_Man_t * p, int idx, char * c);
extern void            ER_GetStats( ER_Man_t * p );
extern void            ER_PrintStats ( ER_Man_t * p );
// helper
extern void            ER_PrintVCount( ER_Man_t * p );

// ER.cpp --------------------
extern void Sample_MC_Miter(Abc_Ntk_t * pNtk, int nKey, int fVerbose);

ABC_NAMESPACE_HEADER_END
#endif