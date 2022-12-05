#include "Hybrid.h"
#include "hyb.h"

#include <math.h>
#include <vector>
#include <stdlib.h>        // rand()

ABC_NAMESPACE_IMPL_START
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static Cut_Man_t * ntkStartCutManForHybrid( Abc_Ntk_t * pNtk );

//// Aux ///////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Known bugs  [ ]
  Synopsis    [ Hybridize network with FTL blocks that represents 
  threshold functions. ]
  Description [ Returns new network with FTL blocks inserted that has the
  same functionality to the original one. ]               
  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t * FTL_Hybrid(Abc_Ntk_t * pNtk, int fVerbose) {
    Cut_Man_t * pManCut;
    Hyb_Man_t * pManHyb;
    abctime clk, clkStart = Abc_Clock();
    
    assert( Abc_NtkIsStrash(pNtk) );
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the hybridize mgr
    pManHyb = Hyb_ManStart(pNtk, fVerbose);
    if(pManHyb == NULL)
        return NULL;
    // start the cut mgr
    pManCut = ntkStartCutManForHybrid( pNtk );
    pNtk->pManCut = pManCut;    

    Hyb_PoCollectCand( pManHyb, fVerbose);

    // print stats
    if(fVerbose)
        Hyb_ManPrintStats( pManHyb );

    // delete the mgr
    Cut_ManStop( pManCut );
    pNtk->pManCut = NULL;
    Hyb_ManStop( pManHyb );

    Abc_NtkCheck(pNtk);

    return pNtk;
}

Cut_Man_t * ntkStartCutManForHybrid( Abc_Ntk_t * pNtk ) {
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = 5;     // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 250;   // the max number of cuts kept at a node
    pParams->fTruth    = 1;     // compute truth tables
    pParams->fFilter   = 0;     // filter dominated cuts
    pParams->fSeq      = 0;     // compute sequential cuts
    pParams->fDrop     = 0;     // drop cuts on the fly
    pParams->fVerbose  = 0;     // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}



ABC_NAMESPACE_IMPL_END