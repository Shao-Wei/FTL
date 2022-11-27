#include "hyb.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Hyb_CutEvaluate( Hyb_Man_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut, Vec_Ptr_t * vFaninsCur, int nNodesSaved);

/**Function*************************************************************
  Synopsis    [ Collect all hybridization candidates of a primary output ]
  Description []               
  SideEffects []
  SeeAlso     []
***********************************************************************/
void Hyb_PoCollectCand( Hyb_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pNode, int fVerbose ) {
    Cut_Cut_t * pCut;
    Abc_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruth;
    char * pPerm;
    int nNodesSaved;
    int i, ret;

    if(!Abc_ObjIsPo(pNode)) {
        printf("Hyb_PoCollectCand: pNode is not a PO. Abort.");
        return;
    }

    // get the node's cuts
    pCut = (Cut_Cut_t *)Abc_NodeGetCutsRecursive( pManCut, pNode, 0, 0 );
    assert( pCut != NULL );

    // go through the cuts
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        // consider only 4-input cuts
        if( pCut->nLeaves < 4) 
            continue;

        if(fVerbose)
            Cut_CutPrint( pCut, 0); printf("\n");

        // get the fanin permutation
        uTruth = 0xFFFF & *Cut_CutReadTruth(pCut);
        pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
        uPhase = p->pPhases[uTruth];
        // collect fanins with the corresponding permutation/phase
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Abc_NtkObj( pNode->pNtk, pCut->pLeaves[(int)pPerm[i]] );
            if ( pFanin == NULL )
                break;
            pFanin = Abc_ObjNotCond(pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
        if ( i != (int)pCut->nLeaves )
        {
            p->nCutsBad++;
            continue;
        }
        p->nCutsGood++;

        // mark the fanin boundary 
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
            Abc_ObjRegular(pFanin)->vFanouts.nSize++;
        // label MFFC with current ID
        Abc_NtkIncrementTravId( pNode->pNtk );
        nNodesSaved = Abc_NodeMffcLabelAig( pNode );
        // unmark the fanin boundary
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
            Abc_ObjRegular(pFanin)->vFanouts.nSize--;

        // evaluate the cut
        ret = Hyb_CutEvaluate( p, pNode, pCut, p->vFaninsCur, nNodesSaved);
    
        // add cut to storage (add to .h)

    }
}






ABC_NAMESPACE_IMPL_END