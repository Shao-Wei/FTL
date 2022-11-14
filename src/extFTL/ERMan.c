#include "ER.h"

ABC_NAMESPACE_IMPL_START

static void calMean(mpz_t ret, Vec_Ptr_t * v);
static void calStdev(mpz_t ret, Vec_Ptr_t * v);

/**Function*************************************************************

  Synopsis    [Start ER manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ER_Man_t * ER_ManStart() {
    ER_Man_t * p;
    p = ABC_ALLOC( ER_Man_t, 1);
    memset( p, 0, sizeof(ER_Man_t) );

    p->vCount = Vec_PtrAlloc(0);
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
    // for(int i=0; i<Vec_PtrSize(p->vCount); i++)
    //     mpz_clear(Vec_PtrGetEntry(p->vCount, i));
    Vec_PtrFree(p->vCount);
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

void ER_CountPush( ER_Man_t * p, char * c) {
    mpz_t num;
    mpz_init_set_str(num, c,10);
    mpz_out_str(stdout, 10, num); // Good til here
    Vec_PtrPush(p->vCount, num);
}

/**Function*************************************************************

  Synopsis    [Get mean & stdev values for vCount]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_GetStats( ER_Man_t * p) {

    // Print all
    printf("Array of number given:\n");
    for(int i=0; i<Vec_PtrSize(p->vCount); i++) {
        printf("  ");
        mpz_out_str(stdout, 10, Vec_PtrGetEntry(p->vCount, i));
        printf("\n");
    }

    calMean(p->mean, p->vCount);
    calStdev(p->stdev, p->vCount);
}

/**Function*************************************************************

  Synopsis    [Print result & timing stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_PrintStats ( ER_Man_t * p ) {
    printf("ER Man Stats:\n");
    printf("  - Number of counts = %i\n", Vec_PtrSize(p->vCount));
    printf("  - Mean = "); mpz_out_str(stdout, 10, p->mean); printf("\n");
    printf("  - Stdev = "); mpz_out_str(stdout, 10, p->stdev); printf("\n");
}

/**Function*************************************************************

  Synopsis    [Testing function for mpz_class usage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ER_mpz_test() {
    mpz_t c1, c2;
    mpz_t mean, stdev;
    Vec_Ptr_t * vCount = Vec_PtrAlloc(0);
    
    mpz_init_set_str(c1, "150000000000000", 10);
    mpz_init_set_str(c2, "870000000000001", 10);
    mpz_init_set_ui(mean, 0);

    Vec_PtrPush(vCount, c1);
    Vec_PtrPush(vCount, c2);

    // Print all
    printf("Array of number given:\n");
    for(int i=0; i<Vec_PtrSize(vCount); i++) {
        printf("  ");
        mpz_out_str(stdout, 10, Vec_PtrGetEntry(vCount, i));
        printf("\n");
    }
    
    calMean(mean, vCount);
    printf("- Mean = ");
    mpz_out_str(stdout, 10, mean);
    printf("\n");

    calStdev(stdev, vCount);
    printf("- Stdev = ");
    mpz_out_str(stdout, 10, stdev);
    printf("\n");
    
    // cleanup
    mpz_clear(c1);
    mpz_clear(c2);
    mpz_clear(mean);
}

void calMean(mpz_t ret, Vec_Ptr_t * v) {
    // Init to 0
    mpz_init_set_si(ret, 0);

    // Sum and divide
    for(int i=0; i<Vec_PtrSize(v); i++)
        mpz_add(ret, ret, Vec_PtrEntry(v, i));
    mpz_cdiv_q_ui(ret, ret, Vec_PtrSize(v));
}

void calStdev(mpz_t ret, Vec_Ptr_t * v) {
    mpz_t mean;
    // Init to 0
    mpz_init_set_si(mean, 0);
    mpz_init_set_si(ret, 0);

    // Get mean
    calMean(mean, v);

    // Sum and divide
    for(int i=0; i<Vec_PtrSize(v); i++)
        mpz_addmul(ret, Vec_PtrEntry(v, i), Vec_PtrEntry(v, i));
    mpz_cdiv_q_ui(ret, ret, Vec_PtrSize(v));
    mpz_submul(ret, mean, mean);
    mpz_sqrt(ret, ret);
}

ABC_NAMESPACE_IMPL_END