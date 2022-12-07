#include "hyb.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Hyb_CutEvaluate( Hyb_Man_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut, Vec_Ptr_t * vFaninsCur, int nNodesSaved);
static unsigned getUMask(int nVar);
static int test_threshold(Hyb_Man_t * p, int nVar, unsigned uTruth);

/**Function*************************************************************
  Synopsis    [ Collect all hybridization candidates of a primary output ]
  Description []               
  SideEffects []
  SeeAlso     []
***********************************************************************/
void Hyb_PoCollectCand( Hyb_Man_t * p, int fVerbose ) {
    Cut_Man_t * pManCut;
    Cut_Cut_t * pCut;
    Abc_Obj_t * pPo, *pNode; // Fanin 0 of PO
    Abc_Obj_t * pFanin;
    Hyb_Cand_t * pCand;
    unsigned uTruth;
    int n, i;
    int nNodesSaved;

    pManCut = p->pNtk->pManCut;
    Abc_NtkForEachPo(p->pNtk, pPo, n) {
        pNode = Abc_ObjFanin0(pPo);

        // get the node's cuts
        pCut = (Cut_Cut_t *)Abc_NodeGetCutsRecursive( pManCut, pNode, 0, 0 );
        assert( pCut != NULL );

        if(fVerbose) { 
            // printf("Collecting candidate from PO %i..\n", n);
            // printf( "Node %s:\n", Abc_ObjName(pNode) );
            // Cut_CutPrintList( pCut, 0 );
        }

        // go through the cuts
        for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
        {
            // consider only cuts w/ less than 5 inputs
            if( pCut->nLeaves < 2 || pCut->nLeaves > 5) 
                continue;
            // get signature
            uTruth = getUMask(pCut->nLeaves) & *Cut_CutReadTruth(pCut);

            // get the fanin
            Vec_PtrClear( p->vFaninsCur );
            Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
            {
                pFanin = Abc_NtkObj( pNode->pNtk, pCut->pLeaves[i] );
                if ( pFanin == NULL )
                    break;
                Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
            }
            if ( i != (int)pCut->nLeaves )
            {
                p->nCutsBad[n]++;
                continue;
            }
            p->nCutsGood[n]++;

            // test threshold
            if(test_threshold(p, pCut->nLeaves, uTruth)) {
                // Get sub-circuit size
                // - mark the fanin boundary 
                Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                    Abc_ObjRegular(pFanin)->vFanouts.nSize++;

                // - label MFFC with current ID
                Abc_NtkIncrementTravId( pNode->pNtk );
                nNodesSaved = Abc_NodeMffcLabelAig( pNode );
                // - unmark the fanin boundary
                Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                    Abc_ObjRegular(pFanin)->vFanouts.nSize--;

                // Add to cand
                pCand = Hyb_ManAddCand(p, n, pCut, nNodesSaved);
                if(pCand == NULL) {
                    printf("Error: failed to add to candidate list\n");
                }
                // Add to stats
                if(pCut->nLeaves == 2)
                    p->nCuts2[n]++;
                else if(pCut->nLeaves == 3)
                    p->nCuts3[n]++;
                else if(pCut->nLeaves == 4)
                    p->nCuts4[n]++;
                else { p->nCuts5[n]++; }
            }
        }
    }
}

unsigned getUMask(int nVar) {
    unsigned ret = 0;
    if(nVar == 2)
        ret = 0xF;
    else if(nVar == 3)
        ret = 0xFF;
    else if(nVar == 4)
        ret = 0xFFFF;
    else if(nVar == 5)
        ret = 0xFFFFFFFF;
    else { assert(0); }

    return ret;
}

// uTruth must be NULL terminated for hash table look up
int test_threshold(Hyb_Man_t * p, int nVar, unsigned uTruth) {
    int ret;
    StrMap * sm; 
    char hex[10];
    //convert unsigned int to hex string
    sprintf(hex, "%x", uTruth);

    // get corresponding hash table, append null to end of string
    if(nVar == 2) {
        sm = p->pTable2;
        hex[1] = '\0';
    }
    else if(nVar == 3) {
        sm = p->pTable3;
        hex[2] = '\0';
    }  
    else if(nVar == 4) {
        sm = p->pTable4;
        hex[4] = '\0';
    }
    else if(nVar == 5) {
        sm = p->pTable5;
        hex[8] = '\0';
    }
    else { assert(0); }
    //check if signature exists in the hash table
    ret = sm_exists(sm, hex);

    return ret;
}

/**Function*************************************************************
  Synopsis    [ Greedy select set of hybridization. ]
  Description []               
  SideEffects []
  SeeAlso     []
***********************************************************************/
void Hyb_CandGreedySelect( Hyb_Man_t * p, int fVerbose ) {
    Abc_Obj_t * pPo, *pNode, * pFanin, *pObj;
    Hyb_Cand_t * pHead;
    Cut_Cut_t * pCut;
    int bMarked;
    int i, n, nNodesSaved;

    // unmark all nodes
    Abc_NtkForEachNode(p->pNtk, pObj, i)
        pObj->fMarkA = 0;

    // clear selected set
    Vec_PtrClear( p->vCand_Greedy );
    Vec_PtrFill( p->vCand_Greedy, Abc_NtkPoNum(p->pNtk), NULL);

    if(fVerbose) {
        int sizeMax, nCand;
        printf("Greedy select - cand list size stats:\n");
        Abc_NtkForEachPo(p->pNtk, pPo, n) {
            pNode = Abc_ObjFanin0(pPo);
            sizeMax = 0; nCand = 0;
            for(pHead = p->pCand[n]; pHead; pHead = pHead->pNext) {
                if(sizeMax == 0)
                    sizeMax = pHead->size;
                nCand++;
            }
            if(nCand > 0)
                printf("  - PO %i: #cand %i, sizeMax %i\n", n, nCand, sizeMax);
        }
    }

    // select for each node
    Abc_NtkForEachPo(p->pNtk, pPo, n) {
        pNode = Abc_ObjFanin0(pPo);
        for(pHead = p->pCand[n]; pHead; pHead = pHead->pNext) {
            pCut = pHead->pCut;
            // get the fanin
            Vec_PtrClear( p->vFaninsCur );
            Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
            {
                pFanin = Abc_NtkObj( p->pNtk, pCut->pLeaves[i] );
                if ( pFanin == NULL )
                    break;
                Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
            }
            if ( i != (int)pCut->nLeaves )
                continue;
            // check if cand conflict the marked area
            // - mark the fanin boundary 
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                Abc_ObjRegular(pFanin)->vFanouts.nSize++;
            // - label MFFC with current ID
            Abc_NtkIncrementTravId( pNode->pNtk );
            nNodesSaved = Abc_NodeMffcLabelAig( pNode );
            // - unmark the fanin boundary
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                Abc_ObjRegular(pFanin)->vFanouts.nSize--;
            // - check marked area
            bMarked = 0;
            Abc_NtkForEachNode(p->pNtk, pObj, i) {
                if(Abc_NodeIsTravIdCurrent(pObj) && pObj->fMarkA == 1) {
                    bMarked = 1;
                    break;
                }
            }
            if(!bMarked)  { // does not conflict w/ marked area
                break;
            }
        }
        if(pHead) {
            // mark affected area
            Abc_NtkForEachNode(pNode->pNtk, pObj, i) {
                if(Abc_NodeIsTravIdCurrent(pObj)) 
                    pObj->fMarkA = 0;
            }
            // add cand to set
            Vec_PtrWriteEntry( p->vCand_Greedy, n, pHead);
        }
    }
}
ABC_NAMESPACE_IMPL_END