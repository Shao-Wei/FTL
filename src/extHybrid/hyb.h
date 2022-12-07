#ifndef ABC__ext__hyb_h
#define ABC__ext__hyb_h

#include "base/abc/abc.h"
#include "opt/cut/cut.h"
#include "strmap.h"

ABC_NAMESPACE_HEADER_START

typedef struct Hyb_Man_t_ Hyb_Man_t;
typedef struct Hyb_Cand_t_ Hyb_Cand_t;

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
    Extra_MmFixed_t *  pMmNode; // memory for nodes

    Hyb_Cand_t ** pPoCand; // #Po lists of valid po hybrid candidates in decending order in size
    Vec_Ptr_t *vPoCand_Greedy; // the greedily selected po cand set, alloc(nPo)
    Hyb_Cand_t ** pPiCand; // single list of valid pi hybrid candidates in decending order in size
    Vec_Ptr_t *vPiCand_Greedy; // the greedily selected pi cand set, alloc(0)

    // cut stats
    // - output hybridization
    int *nCutsGood; // array of number of good cuts of each po
    int *nCutsBad; // array of number of bad cuts of each po
    int *nCuts2; // array of number of 2 input threshold function of each po
    int *nCuts3; // array of number of 3 input threshold function of each po
    int *nCuts4; // array of number of 4 input threshold function of each po
    int *nCuts5; // array of number of 5 input threshold function of each po
    // - input hybridization
    int nPiCutsGood, nPiCutsBad, nPiCuts2, nPiCuts3, nPiCuts4, nPiCuts5;
    
    // runtime statistics
    abctime timeCollectCand;
    abctime timeGreedySelect;
    abctime timeTotal;
};

struct Hyb_Cand_t_
{
    Cut_Cut_t * pCut;
    int size; // size of sub-circuit in aig
    Hyb_Cand_t * pPrev; // prev in the list
    Hyb_Cand_t * pNext; // next in the list
};

// hybMan.c
extern Hyb_Man_t *      Hyb_ManStart(Abc_Ntk_t * pNtk, int fVerbose);
extern void             Hyb_ManStop( Hyb_Man_t * p );

extern void             Hyb_ManPrintPoCandStats ( Hyb_Man_t * p );
extern void             Hyb_ManPrintPoGreedyResult( Hyb_Man_t * p );
extern void             Hyb_ManPrintPiCandStats ( Hyb_Man_t * p );
extern void             Hyb_ManPrintPiGreedyResult( Hyb_Man_t * p );
extern void             Hyb_ManPrintTimeStats( Hyb_Man_t * p );

// hybLib.c
extern Hyb_Cand_t *     Hyb_ManAddPoCand( Hyb_Man_t * p, int nPo, Cut_Cut_t * pCut, int size);
extern Hyb_Cand_t *     Hyb_ManAddPiCand( Hyb_Man_t * p, Cut_Cut_t * pCut, int size);

// hybEva.c
extern void             Hyb_PoCollectCand( Hyb_Man_t * p, int fVerbose );
extern void             Hyb_PoCandGreedySelect( Hyb_Man_t * p, int fVerbose );
extern void             Hyb_PiCollectCand( Hyb_Man_t * p, int fVerbose);
extern void             Hyb_PiCandGreedySelect( Hyb_Man_t * p, int fVerbose);


ABC_NAMESPACE_HEADER_END
#endif