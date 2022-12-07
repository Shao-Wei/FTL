#include "hyb.h"

ABC_NAMESPACE_IMPL_START
/**Function*************************************************************

  Synopsis    [ Create and add one candidate to  po cand list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hyb_Cand_t * Hyb_ManAddPoCand( Hyb_Man_t * p, int nPo, Cut_Cut_t * pCut, int size) {
    Hyb_Cand_t * pNew;
    Hyb_Cand_t * pHead, *pPrev = NULL;

    pNew = (Hyb_Cand_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->pCut = pCut;
    pNew->size = size;
    pNew->pPrev = NULL;
    pNew->pNext = NULL;

    // add to linked list in decending order
    if(p->pPoCand[nPo] == NULL) {
        p->pPoCand[nPo] = pNew; // add as head
    }
    else {
        for ( pHead = p->pPoCand[nPo]; pHead; pHead = pHead->pNext ) { // add to position
            pPrev = pHead->pPrev;
            if(pHead->size <= size)
                break;
        }
        // insert before pHead
        if(pPrev == NULL) { // add to head
            p->pPoCand[nPo] = pNew;
            pHead->pPrev = pNew;
            pNew->pNext = pHead;
        }
        else if(pHead == NULL) { // add to end
            pPrev->pNext = pNew;
            pNew->pPrev = pPrev;
        }
        else {
            pPrev->pNext = pNew;
            pNew->pPrev = pPrev;
            pHead->pPrev = pNew;
            pNew->pNext = pHead;
        }
    }

    return pNew;
}

/**Function*************************************************************

  Synopsis    [ Create and add one candidate to pi cand list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hyb_Cand_t * Hyb_ManAddPiCand( Hyb_Man_t * p, Cut_Cut_t * pCut, int size) {
    Hyb_Cand_t * pNew;
    Hyb_Cand_t * pHead, *pPrev = NULL;

    pNew = (Hyb_Cand_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->pCut = pCut;
    pNew->size = size;
    pNew->pPrev = NULL;
    pNew->pNext = NULL;

    // add to linked list in decending order
    if(p->pPiCand[0] == NULL) {
        p->pPiCand[0] = pNew; // add as head
    }
    else {
        for ( pHead = p->pPiCand[0]; pHead; pHead = pHead->pNext ) { // add to position
            pPrev = pHead->pPrev;
            if(pHead->size <= size)
                break;
        }
        // insert before pHead
        if(pPrev == NULL) { // add to head
            p->pPiCand[0] = pNew;
            pHead->pPrev = pNew;
            pNew->pNext = pHead;
        }
        else if(pHead == NULL) { // add to end
            pPrev->pNext = pNew;
            pNew->pPrev = pPrev;
        }
        else {
            pPrev->pNext = pNew;
            pNew->pPrev = pPrev;
            pHead->pPrev = pNew;
            pNew->pNext = pHead;
        }
    }

    return pNew;
}

ABC_NAMESPACE_IMPL_END