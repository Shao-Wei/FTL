#include "ER.h"

ABC_NAMESPACE_IMPL_START
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static float getPercentage(mpz_t count, int nBit);

/**Function*************************************************************

  Synopsis    [Start ER manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ER_Man_t * ER_ManStart(int nPi, int nKey) {
    ER_Man_t * p;
    p = ABC_ALLOC( ER_Man_t, 1);
    memset( p, 0, sizeof(ER_Man_t) );

    p->nPi = nPi;
    p->nKey = nKey;
    p->vCountSize = (1<<nKey) - 1;
    p->vCount = malloc(p->vCountSize * sizeof(mpz_t));
    for(int i=0; i<p->vCountSize; i++) 
        mpz_init(p->vCount[i]);
    mpz_init(p->mean);
    mpz_init(p->stdev);
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop ER manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_ManStop( ER_Man_t * p ) {
    for(int i=0; i<p->vCountSize; i++) 
        mpz_clear(p->vCount[i]);
    free(p->vCount);
    mpz_clear(p->mean);
    mpz_clear(p->stdev);
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Push single counting result into vCount.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void ER_AddCount( ER_Man_t * p, int idx, char * c) {
    if(idx >= 0 && idx < p->vCountSize)
        mpz_set_str(p->vCount[idx], c, 10);
}

/**Function*************************************************************

  Synopsis    [ Get mean & stdev values for vCount. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_GetStats( ER_Man_t * p) {
    // Cal mean
    mpz_init_set_si(p->mean, 0);
    for(int i=0; i<p->vCountSize; i++)
        mpz_add(p->mean, p->mean, p->vCount[i]);
    mpz_cdiv_q_ui(p->mean, p->mean, p->vCountSize);

    // Cal stdev
    mpz_init_set_si(p->stdev, 0);
    for(int i=0; i<p->vCountSize; i++)
        mpz_addmul(p->stdev, p->vCount[i], p->vCount[i]);
    mpz_cdiv_q_ui(p->stdev, p->stdev, p->vCountSize);
    mpz_submul(p->stdev, p->mean, p->mean);
    mpz_sqrt(p->stdev, p->stdev);
}

/**Function*************************************************************

  Synopsis    [ Print all entries in vCount. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_PrintVCount( ER_Man_t * p ) {
    printf("vCount:\n");
    for(int i=0; i<p->vCountSize; i++) {
        printf("  ");
        mpz_out_str(stdout, 10, p->vCount[i]);
        printf("\n");
    }
}

/**Function*************************************************************

  Synopsis    [Print result & timing stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_PrintStats ( ER_Man_t * p ) {
    printf("\n# ER Man Stats:\n");
    printf("  - Number of counts = %i\n", p->vCountSize);
    printf("  - Mean = %.3f%% ( ", getPercentage(p->mean, p->nPi)); mpz_out_str(stdout, 10, p->mean); printf(" over 2^%i)\n", p->nPi);
    printf("  - Stdev = %.3f%% ( ", getPercentage(p->stdev, p->nPi)); mpz_out_str(stdout, 10, p->stdev); printf(" over 2^%i)\n", p->nPi);
}

// Return number of percentage of count proportional to the nBit Boolean space
float getPercentage(mpz_t count, int nBit) {
    mpz_t mTmp;
    long l;
    float f;
    mpz_init_set(mTmp, count);

    // (float) count * 1000 / 2 ^ nBit
    mpz_mul_ui(mTmp, mTmp, 1000);
    mpz_fdiv_q_2exp(mTmp, mTmp, nBit);
    l = mpz_get_ui(mTmp);
    f = (float)l / 1000.0;

    // clean up
    mpz_clear(mTmp);
    return f;
}

ABC_NAMESPACE_IMPL_END