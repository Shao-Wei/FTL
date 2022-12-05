
#include "hyb.h"
#include "base/main/main.h"
#include "bool/dec/dec.h"
#include <sys/types.h> // ssize_t
#include <math.h> // sqrt

ABC_NAMESPACE_IMPL_START

static int readHashTableFile(Hyb_Man_t * p, int fVerbose);
/**Function*************************************************************

  Synopsis    [Starts hybridization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hyb_Man_t * Hyb_ManStart(Abc_Ntk_t * pNtk, int fVerbose) {
    int ret;
    Hyb_Man_t * p;
    int i;

    p = ABC_ALLOC( Hyb_Man_t, 1 );
    memset( p, 0, sizeof(Hyb_Man_t) );
    p->pNtk = pNtk;
    // create the table
    ret = readHashTableFile(p, fVerbose);
    if(!ret) {
      printf("Unable to read precomputed hash table. Hyb_ManStart() abort.\n");
      return NULL;
    }

    p->nTravIds = 1;
    p->vFaninsCur = Vec_PtrAlloc( 50 );

    // cut stats
    p->nCutsGood = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    p->nCutsBad  = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    p->nCuts2    = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    p->nCuts3    = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    p->nCuts4    = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    p->nCuts5    = ABC_ALLOC(int, Abc_NtkPoNum(pNtk));
    for(i=0; i<Abc_NtkPoNum(pNtk); i++) {
      p->nCutsGood[i] = 0;
      p->nCutsBad[i] = 0;
      p->nCuts2[i] = 0;
      p->nCuts3[i] = 0;
      p->nCuts4[i] = 0;
      p->nCuts5[i] = 0;
    }

    return p;
}

int readHashTableFile(Hyb_Man_t * p, int fVerbose) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    p->nFunc2 = 16;
    fp = fopen("src/extHybrid/hash_tables/np/hash2np.txt", "r");
    if(fp == NULL)
      return 0;
    p->pTable2 = sm_new(p->nFunc2);
    while ((read = getline(&line, &len, fp)) != -1) {
        line[strlen(line)-1] = '\0';
        sm_put(p->pTable2, line, "1");
    }
    fclose(fp);
    // if(fVerbose) { printf("2-var hash table precomputed (%i/%i)\n", sm_get_count(p->pTable2), p->nFunc2); }

    p->nFunc3 = 104;
    fp = fopen("src/extHybrid/hash_tables/np/hash3np.txt", "r");
    if(fp == NULL)
      return 0;
    p->pTable3 = sm_new(p->nFunc3);
    while ((read = getline(&line, &len, fp)) != -1) {
        line[strlen(line)-1] = '\0';
        sm_put(p->pTable3, line, "1");
    }
    fclose(fp);
    // if(fVerbose) { printf("3-var hash table precomputed (%i/%i)\n", sm_get_count(p->pTable3), p->nFunc3); }
    
    p->nFunc4 = 1982;
    fp = fopen("src/extHybrid/hash_tables/np/hash4np.txt", "r");
    if(fp == NULL)
      return 0;
    p->pTable4 = sm_new(p->nFunc4);
    while ((read = getline(&line, &len, fp)) != -1) {
        line[strlen(line)-1] = '\0';
        sm_put(p->pTable4, line, "1");
    }
    fclose(fp);
    // if(fVerbose) { printf("4-var hash table precomputed (%i/%i)\n", sm_get_count(p->pTable4), p->nFunc4); }
    
    p->nFunc5 = 97052;
    fp = fopen("src/extHybrid/hash_tables/np/hash5np.txt", "r");
    if(fp == NULL)
      return 0;
    p->pTable5 = sm_new(p->nFunc5);
    while ((read = getline(&line, &len, fp)) != -1) {
        line[strlen(line)-1] = '\0';
        sm_put(p->pTable5, line, "1");
    }
    fclose(fp);
    // if(fVerbose) { printf("5-var hash table precomputed (%i/%i)\n", sm_get_count(p->pTable5), p->nFunc5); }

    // clean up
    if(line)
      free(line);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Stops hybridization manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hyb_ManStop( Hyb_Man_t * p ) {
    p->pNtk = NULL;
    sm_delete(p->pTable2);
    sm_delete(p->pTable3);
    sm_delete(p->pTable4);
    sm_delete(p->pTable5); 

    Vec_PtrFree( p->vFaninsCur );

    ABC_FREE(p->nCutsGood);
    ABC_FREE(p->nCutsBad); 
    ABC_FREE(p->nCuts2);
    ABC_FREE(p->nCuts3);
    ABC_FREE(p->nCuts4);
    ABC_FREE(p->nCuts5);

    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Print stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hyb_ManPrintStats ( Hyb_Man_t * p ) {
  int c, i, n = Abc_NtkPoNum(p->pNtk);
  int nAcc3[n], nAcc4[n], nAcc5[n]; // accumulated counts of threshold functions til n-cut
  int nHyb3, nHyb4, nHyb5; // number of po hybridized
  double nAvg3, nAvg4, nAvg5; // mean over all po
  double nAvgV3, nAvgV4, nAvgV5; // mean over po that are hybridized
  double nStd3, nStd4, nStd5; // stdiv over all po
  double nStdV3, nStdV4, nStdV5; // stdiv over po that are hybridized

  // get accumulated arrays
  for(i=0; i<n; i++) {
    c = p->nCuts2[i];
    nAcc3[i] = c; nAcc4[i] = c; nAcc5[i] = c;    
  }
  for(i=0; i<n; i++) {
    c = p->nCuts3[i];
    nAcc3[i] += c; nAcc4[i] += c; nAcc5[i] += c;    
  }
  for(i=0; i<n; i++) {
    c = p->nCuts4[i];
    nAcc4[i] += c; nAcc5[i] += c;    
  }
  for(i=0; i<n; i++) {
    c = p->nCuts5[i];
    nAcc5[i] += c;    
  }
  // nPo hybridized
  nHyb3 = nHyb4 = nHyb5 = 0;
  for(i=0; i<n; i++) {
    if(nAcc3[i] > 0)
      nHyb3++;
    if(nAcc4[i] > 0)
      nHyb4++;
    if(nAcc5[i] > 0)
      nHyb5++;
  }

  // mean
  nAvg3 = nAvg4 = nAvg5 = 0;
  for(i=0; i<n; i++) {
    nAvg3 += nAcc3[i];
    nAvg4 += nAcc4[i];
    nAvg5 += nAcc5[i];
  }
  nAvg3 = nAvg3 / (double)n;
  nAvg4 = nAvg4 / (double)n;
  nAvg5 = nAvg5 / (double)n;

  nAvgV3 = nAvgV4 = nAvgV5 = 0.0;
  for(i=0; i<n; i++) {
    nAvgV3 += (nAcc3[i] > 0)? nAcc3[i]: 0;
    nAvgV4 += (nAcc4[i] > 0)? nAcc4[i]: 0;
    nAvgV5 += (nAcc5[i] > 0)? nAcc5[i]: 0;
  }
  nAvgV3 = nAvgV3 / (double)nHyb3;
  nAvgV4 = nAvgV4 / (double)nHyb4;
  nAvgV5 = nAvgV5 / (double)nHyb5;
  // stdiv
  nStd3 = nStd4 = nStd5 = 0.0;
  for(i=0; i<n; i++) {
    nStd3 += (nAcc3[i]-nAvg3) * (nAcc3[i]-nAvg3);
    nStd4 += (nAcc4[i]-nAvg4) * (nAcc4[i]-nAvg4);
    nStd5 += (nAcc5[i]-nAvg5) * (nAcc5[i]-nAvg5);
  }
  nStd3 = sqrt(nStd3 / (double)n);
  nStd4 = sqrt(nStd4 / (double)n);
  nStd5 = sqrt(nStd5 / (double)n);

  nStdV3 = nStdV4 = nStdV5 = 0.0;
  for(i=0; i<n; i++) {
    nStdV3 += (nAcc3[i] > 0)? (nAcc3[i]-nAvg3) * (nAcc3[i]-nAvg3): 0;
    nStdV4 += (nAcc4[i] > 0)? (nAcc4[i]-nAvg4) * (nAcc4[i]-nAvg4): 0;
    nStdV5 += (nAcc5[i] > 0)? (nAcc5[i]-nAvg5) * (nAcc5[i]-nAvg5): 0;
  }
  nStdV3 = sqrt(nStdV3 / (double)nHyb3);
  nStdV4 = sqrt(nStdV4 / (double)nHyb4);
  nStdV5 = sqrt(nStdV5 / (double)nHyb5);

  printf("Hybridization stats:\n");
  printf("Calulated over all PO\n");
  printf("Up to 3-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb3, n, nAvg3, nStd3);
  printf("Up to 4-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb4, n, nAvg4, nStd4);
  printf("Up to 5-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb5, n, nAvg5, nStd5);
  printf("Calculated over po having hyb opportunities\n");
  printf("Up to 3-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb3, n, nAvgV3, nStdV3);
  printf("Up to 4-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb4, n, nAvgV4, nStdV4);
  printf("Up to 5-cut: #PO %i/%i, #valid cuts mean %.3f, stdiv %.3f\n", nHyb5, n, nAvgV5, nStdV5);
  /**
  printf("\n  - nCutsGood:");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCutsGood[i]);
  printf("\n  - nCutsBad: ");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCutsBad[i]);
  printf("\n  - nCuts2:   ");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCuts2[i]);
  printf("\n  - nCuts3:   ");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCuts3[i]);
  printf("\n  - nCuts4:   ");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCuts4[i]);
  printf("\n  - nCuts5:   ");
  for(i=0; i<n; i++)
    printf(" %3i", p->nCuts5[i]);
  printf("\n");
  */
}



ABC_NAMESPACE_IMPL_END