#include "hyb.h"

ABC_NAMESPACE_IMPL_START
/**Function*************************************************************

  Synopsis    [ Create and add one candidate to cand list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hyb_Cand_t * Hyb_ManAddCand( Hyb_Man_t * p, int nPo, Cut_Cut_t * pCut, int size) {
    Hyb_Cand_t * pNew;
    Hyb_Cand_t * pHead, *pPrev;

    pNew = (Hyb_Cand_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->pCut = pCut;
    pNew->size = size;
    pNew->pPrev = NULL;
    pNew->pNext = NULL;

    // add to linked list in decending order
    if(p->pCand[nPo] == NULL) {
        p->pCand[nPo] = pNew; // add as head
    }
    else {
        for ( pHead = p->pCand[nPo]; pHead; pHead = pHead->pNext ) { // add to position
            pPrev = pHead->pPrev;
            if(pHead->size <= size)
                break;
        }
        // insert before pHead
        if(pPrev == NULL) { // add to head
            p->pCand[nPo] = pNew;
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