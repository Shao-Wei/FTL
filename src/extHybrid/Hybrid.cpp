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
Cut_Man_t * ntkStartCutManForHybrid( Abc_Ntk_t * pNtk );

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
    ProgressBar * pProgress;
    Cut_Man_t * pManCut;
    Hyb_Man_t * pManHyb;
    Abc_Obj_t * pPo;
    int i, nGain;
    abctime clk, clkStart = Abc_Clock();
    
    assert( Abc_NtkIsStrash(pNtk) );
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the hybridize mgr
    pManHyb = Hyb_ManStart();
    if(pManHyb == NULL)
        return NULL;
    // start the cut mgr
    pManCut = ntkStartCutManForHybrid( pNtk );
    pNtk->pManCut = pManCut;    

    Abc_NtkForEachPo( pNtk, pPo, i)
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // for each cut, try to hybridize it
        nGain = Hyb_PoHybridize( pManHyb, pManCut, pPo);
        if( !nGain )
            continue;
        // if we end up here, a hybridize step is accepted

        // get hold of the new subgraph to be added to the AIG
        // TODO
    }
    Extra_ProgressBarStop( pProgress );

    // print stats
    if(fVerbose)
        Hyb_ManPrintStats( pManHyb );

    // delete the mgr
    Hyb_ManStop( pManHyb );
    Cut_ManStop( pManCut );
    pNtk->pManCut = NULL;

    return NULL;
}



ABC_NAMESPACE_IMPL_END