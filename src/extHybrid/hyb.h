#ifndef ABC__ext__hyb_h
#define ABC__ext__hyb_h

#include "base/abc/abc.h"
#include "opt/cut/cut.h"
#include "strmap.h"

ABC_NAMESPACE_HEADER_START

typedef struct Hyb_Man_t_ Hyb_Man_t;

struct Hyb_Man_t_
{
    Abc_Ntk_t * pNtk;
    // internal lookups
    int nFunc2; // number of 2 var threshold functions
    StrMap * pTable2; // 2 var threshold function lookup
    int nFunc3; // number of 3 var threshold functions
    StrMap * pTable3; // 3 var threshold function lookup
    int nFunc4; // number of 4 var threshold functions
    StrMap * pTable4; // 4 var threshold function lookup
    int nFunc5; // number of 5 var threshold functions
    StrMap * pTable5; // 5 var threshold function lookup

    // Statistical var
    int nTravIds; // the counter of traversal IDs
    
    // resyn result
    Vec_Ptr_t *vFaninsCur; // the fanins array (temporary)

    // cut stats
    int *nCutsGood; // array of number of good cuts of each po
    int *nCutsBad; // array of number of bad cuts of each po
    int *nCuts2; // array of number of 2 input threshold function of each po
    int *nCuts3; // array of number of 3 input threshold function of each po
    int *nCuts4; // array of number of 4 input threshold function of each po
    int *nCuts5; // array of number of 5 input threshold function of each po

    // runtime statistics
};

// hybMan.c
extern Hyb_Man_t *      Hyb_ManStart(Abc_Ntk_t * pNtk, int fVerbose);
extern void             Hyb_ManStop( Hyb_Man_t * p );
extern void             Hyb_ManPrintStats ( Hyb_Man_t * p );

// hybEva.c
extern void             Hyb_PoCollectCand( Hyb_Man_t * p, Cut_Man_t * pManCut, int fVerbose );


ABC_NAMESPACE_HEADER_END
#endif