
#include "hyb.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Starts hybridization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hyb_Man_t * Hyb_ManStart() {
    p = ABC_ALLOC( Hyb_Man_t, 1 );
    memset( p, 0, sizeof(Hyb_Man_t) );

    return p;
}

/**Function*************************************************************

  Synopsis    [Stops hybridization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hyb_ManStop( Rwr_Man_t * p ) {
    ABC_FREE( p );
}




ABC_NAMESPACE_IMPL_END