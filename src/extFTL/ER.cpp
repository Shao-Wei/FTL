#include "ER.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include <math.h>
#include <vector>
#include <stdlib.h>        // rand()

ABC_NAMESPACE_IMPL_START
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
void insertKey(Abc_Ntk_t* pNtk, int nKey, int seed, int& correctKey);

void Sample_MCInt(Abc_Ntk_t * pNtk, int nPi, int nKey, int nPo, char* cnfFileName);
Abc_Ntk_t * Sample_MC_MiterInt(Abc_Ntk_t * pNtk, int nKey);
static void ntkMiterPrepare( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter, int nPi, int nKey);
static void ntkMiterAddOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter);
static void ntkMiterFinalize( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter);
static void Write_File_Miter_Counting( Abc_Ntk_t * pNtkMiter, char * fileName, int nPi, int nKey);
static void Write_File_Miter_Counting_Header(char * fileName);

void AddKeyInfo2CNF(char* cnfFileName, int correctKey, int wrongKey, int nKey);

//// Aux ///////////////////////////////////////////////////////////////
/*== base/abci/abcDar.c ==*/
extern "C" { Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters); }
// extern "C" {Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor ); }

char * int2bitstring(int value, int length);

void write_clause_to_file(FILE* ff, int& nClause, lit* begin, lit* end);
int sat_solver_conditional_unequal(FILE* ff, int& nClause,  sat_solver * pSat, int iVar, int iVar2, int iVarCond );
int sat_solver_iff_unequal(FILE* ff, int& nClause, sat_solver *pSat, int iVar, int iVar2, int iVarCond);
int sat_solver_and(FILE* ff, int& nClause, sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl );
int sat_solver_buffer(FILE* ff, int& nClause, sat_solver * pSat, int iVarA, int iVarB, int fCompl );
int sat_solver_const(FILE* ff, int& nClause, sat_solver * pSat, int iVar, int fCompl );
int sat_solver_addclause_from( sat_solver* pSat, Cnf_Dat_t * pCnf );
void sat_solver_print( sat_solver* pSat, int fDimacs );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Known bugs  [Circuit to CNF formulation incorrect. ]
  Synopsis    [ Sample output corrupted minterms regarding different wrong
  keys through model counting. The construction of miter is through additional
  CNF clauses. ]

  Description [Prints out statistics over the output corruption behavior.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sample_MC(Abc_Ntk_t * pNtk, int nKey) {
    char Command[1000];
    char cnfFileName[1000];
    char resultFileName[1000];
    
    sprintf( cnfFileName, "C432.dimacs");
    sprintf( resultFileName, "MCResult.txt");
    int nPi = Abc_NtkCiNum(pNtk);
    int nPo = Abc_NtkCoNum(pNtk);
    int correctKey = 0;
    int seed = 5;
    std::vector<int> pCount;

    int res = 0; // Suppress warn_unused_result

    // Create general miter
    insertKey(pNtk, nKey, seed, correctKey);
    // Io_Write( pNtk, "keyInserted.blif", IO_FILE_BLIF );

    Sample_MCInt(pNtk, nPi, nKey, nPo, cnfFileName);
    // AddKeyInfo2CNF(cnfFileName, correctKey, 1, nKey);
    //// approxmc 
    printf("Invoke approxmc...\n");
    sprintf( Command, "./approxmc %s > %s", cnfFileName, resultFileName);
    res = system( Command );
    if(res) {
        printf("Approxmc execute with error. Aborted.\n");
    }

    FILE* pf = NULL;
    int count = 0;
    pf = fopen(resultFileName, "r");
    if(!pf) { printf("Error: Cannot open mc result file."); }
    while(fscanf( pf, "%d", &count ) == 1);
    fclose(pf);
    printf("count = %i\n", count);
    
    /**
    // Exhaustive counting
    FILE* pf = NULL;
    int count = 0;
    for(int wrongKey=0; wrongKey<pow(2, nPi); wrongKey++) {
        if(wrongKey == correctKey)
            continue;
        AddKeyInfo2CNF(cnfFileName, correctKey, wrongKey, nKey);
        
        //// approxmc 
        sprintf( Command, "./approxmc %s >> %s", cnfFileName, resultFileName);
        system( Command );
        pf = NULL;
        count = 0;
        pf = fopen(resultFileName, "r");
        if(!pf) { printf("Error: Cannot open mc result file."); }
        while(fscanf( pf, "%d", &count ) == 1);
        fclose(pf);
        pCount.push_back(count);
    }
    **/
}

/**Function*************************************************************

  Synopsis    [ Sample output corrupted minterms regarding different wrong
  keys through model counting. The construction of miter is through building
  miter circuit. ]

  Description [Prints out statistics over the output corruption behavior.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sample_MC_Miter(Abc_Ntk_t * pNtk, int nKey) {
    // int fComb        = 1; // toggles deriving combinational miter
    // int fCheck       = 1; // toggles network check, should always be true
    // int fImplic      = 0; // toggles deriving implication miter
    // int fMulti       = 1; // toggles creating multi-output miter
    // int nPartSize    = 0; // output partition size
    // int fTrans       = 0; // toggle XORing pair-wise POs of the miter
    // int fIgnoreNames = 0; // toggle ignoring names when matching CIs/COs 

    Abc_Ntk_t * pNtkTemp, *pNtkMiter;
    int seed = 5;
    int correctKey = 0;
    int nPi = Abc_NtkCiNum(pNtk);
    char miterFileName[1000];
    sprintf(miterFileName, "TMP");

    // Insert keys
    insertKey(pNtk, nKey, seed, correctKey);

    // Make sure network is strashed
    if ( !Abc_NtkIsStrash(pNtk) ) {
        pNtkTemp = Abc_NtkStrash( pNtk, 0, 1, 0 );
        Abc_NtkDelete(pNtk);
        pNtk = pNtkTemp;
    }

    // Compute the miter
    pNtkMiter = Sample_MC_MiterInt(pNtk, nKey);
    if( pNtkMiter == NULL ) {
        Abc_Print( -1, "Miter computation has failed.\n" );
        return;
    }

    // Network to CNF
    Write_File_Miter_Counting( pNtkMiter, miterFileName, nPi, nKey);
    
    // Do the rest
    
}

// Insert XOR keys into network. Correct key combination is given through var correctKey.
void insertKey(Abc_Ntk_t* pNtk, int nKey, int seed, int& correctKey) {
    Abc_Obj_t *pObj, *pFanout, *pNew;
    int k;
    std::vector<Abc_Obj_t*> vKey;
    char piName[1024];
    // create key inputs
    for(int i=0; i<nKey; i++) {
        pObj = Abc_NtkCreatePi(pNtk);
        sprintf(piName, "KEY_%i", i);
        Abc_ObjAssignName(pObj, piName, NULL);
        vKey.push_back(pObj);
    }
    // insert XOR
    srand((unsigned)seed);
    for(int i=0; i<nKey; i++) {
        pObj = NULL;
        while(pObj == NULL) {
            pObj = Abc_NtkObj(pNtk, rand()%Abc_NtkNodeNum(pNtk));
            if(pObj == NULL || Abc_ObjIsPo(pObj))
                pObj = NULL;
        }
        // assert(!Abc_ObjIsPo(pObj));
        Abc_NtkDfs(pNtk, 0);
        // cut fanouts
        Vec_Ptr_t* vFanouts = Vec_PtrAlloc(0);
        Abc_ObjForEachFanout(pObj, pFanout, k) {
            Vec_PtrPush(vFanouts, pFanout);
        }
        // connect fanouts
        Vec_Ptr_t * vFanins = Vec_PtrAlloc(0);
        Vec_PtrPush(vFanins, vKey[i]);
        Vec_PtrPush(vFanins, pObj);
        pNew = Abc_NtkCreateNodeExor(pNtk, vFanins);
        Vec_PtrForEachEntry(Abc_Obj_t*, vFanouts, pFanout, k) {
            Abc_ObjPatchFanin(pFanout, pObj, pNew);
        }
        Vec_PtrFree(vFanouts);
        Vec_PtrFree(vFanins);
    }
    correctKey = 0;
}

// Construct CNF file of the general miter with correct key & wrong key asserted to be all zeros.
void Sample_MCInt(Abc_Ntk_t * pNtk, int nPi, int nKey, int nPo, char* cnfFileName)
{   
    FILE* ff = NULL;
    int nVar = 0, nClause = 0;
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);

    Abc_Ntk_t *pNtkOut1 = Abc_NtkDup(pNtk);
    Abc_Ntk_t *pNtkOut2 = Abc_NtkDup(pNtkOut1);

    Vec_Int_t *pOut1Pi = Vec_IntAlloc(nPi);
    Vec_Int_t *pOut1Key = Vec_IntAlloc(nKey);
    Vec_Int_t *pOut1Po = Vec_IntAlloc(nPo);
    {
        Aig_Man_t *pAigOut1 = Abc_NtkToDar(pNtkOut1, 0, 0);
        Cnf_Dat_t *pCnfOut1 = Cnf_Derive(pAigOut1, Abc_NtkCoNum(pNtkOut1));
        Cnf_DataLift(pCnfOut1, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut1);
        // store var of pi
        for (int i = 0; i < nPi; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCi(pCnfOut1->pMan, i);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjOut1)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut1Pi, var);
        }
        // store var of key
        for (int i = nPi; i < nPi+nKey; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCi(pCnfOut1->pMan, i);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjOut1)];
            // Abc_Print(1, "var of key %d: %d\n", i, var);
            Vec_IntPush(pOut1Key, var);
        }
        // store var of po
        for (int i = 0; i < nPo; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCo(pCnfOut1->pMan, i);
            Aig_Obj_t *pAigObjFanin1 = Aig_ObjFanin0(pAigObjOut1);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjFanin1)];
            // Abc_Print(1, "var of po %d: %d\n", i, var);
            Vec_IntPush(pOut1Po, var);
        }
        // memory free
        Cnf_DataFree(pCnfOut1);
        Aig_ManStop(pAigOut1);
    }

    Vec_Int_t *pOut2Pi = Vec_IntAlloc(nPi);
    Vec_Int_t *pOut2Key = Vec_IntAlloc(nKey);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(nPo);
    {
        Aig_Man_t *pAigOut2 = Abc_NtkToDar(pNtkOut2, 0, 0);
        Cnf_Dat_t *pCnfOut2 = Cnf_Derive(pAigOut2, Abc_NtkCoNum(pNtkOut2));
        Cnf_DataLift(pCnfOut2, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut2);
        // store var of pi
        for (int i = 0; i < nPi; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCi(pCnfOut2->pMan, i);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjOut2)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut2Pi, var);
        }
        // store var of key
        for (int i = nPi; i < nPi+nKey; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCi(pCnfOut2->pMan, i);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjOut2)];
            // Abc_Print(1, "var of key %d: %d\n", i, var);
            Vec_IntPush(pOut2Key, var);
        }
        // store var of po
        for (int i = 0; i < nPo; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCo(pCnfOut2->pMan, i);
            Aig_Obj_t *pAigObjFanin2 = Aig_ObjFanin0(pAigObjOut2);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjFanin2)];
            // Abc_Print(1, "var of po %d: %d\n", i, var);
            Vec_IntPush(pOut2Po, var);
        }
        // memory free
        Cnf_DataFree(pCnfOut2);
        Aig_ManStop(pAigOut2);
    }

    // two copies share same inputs
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(nPi);
    for (int i = 0; i < nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_add_buffer_enable(pSolver, varOut1, varOut2, varAlphaControl, 0);
        assert(cid);
        cid = sat_solver_add_const(pSolver, Vec_IntEntry(pAlphaControls, i), 0);
        assert(cid);
    }

    // at least one of the output is different
    Vec_Int_t *pGammaControls = Vec_IntAlloc(nPo);
    for(int i=0; i<nPo; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Po, i);
        int varOut2 = Vec_IntEntry(pOut2Po, i);
        int varGammaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pGammaControls, varGammaControl);
        cid = sat_solver_iff_unequal(ff, nClause, pSolver, varOut1, varOut2, varGammaControl); // self defined
        assert(cid);
    }
    lit Lits_Gamma[nPo];
    for(int i=0; i<nPo; i++)
    {
        int var = Vec_IntEntry(pGammaControls, i);
        Lits_Gamma[i] = toLitCond(var, 0);
    }
    cid = sat_solver_addclause(pSolver, Lits_Gamma, Lits_Gamma + nPo);
    assert(cid);
    // sat_solver_print(pSolver, 1);
    
    // write results
    printf("Sampled set (i.e. input set)\n");
    for(int i=0; i<nPi; i++)
        printf("%i ", Vec_IntEntry(pOut1Pi, i)+1);
    printf("\n");
    printf("Key variables - two copies\n");
    for(int i=0; i<nKey; i++)
        printf("%i ", Vec_IntEntry(pOut1Key, i)+1);
    printf("\n");
    for(int i=0; i<nKey; i++)
        printf("%i ", Vec_IntEntry(pOut2Key, i)+1);
    printf("\n");
    
    // Note: option fIncrement adds an 0 to the end of each line, and to accomplish that, 
    // every variable index is also incremented. Thus, when modifying the output files directly,
    // do not forget to increment the target variable index.
    Sat_SolverWriteDimacs(pSolver, cnfFileName, NULL, NULL, 1);

    // write header & assumptions for approxmc
    char tmpFileName[1000];
    sprintf(tmpFileName, "miterToBeReplaced.dimacs");
    int buffL = 1024;
    char buff[buffL];
    char * t;
    char * line __attribute__((unused)); // get rid of fget warnings
    FILE* fRead = fopen(cnfFileName, "r");
    FILE* fWrite = fopen(tmpFileName, "w");
    fprintf(fWrite, "c ind"); // c ind <sampled set>
    for(int i=0; i<nPi; i++)
        fprintf(fWrite, " %i", Vec_IntEntry(pOut1Pi, i)+1);
    fprintf(fWrite, " 0\n");
    line = fgets(buff, buffL, fRead); // p cnf
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n"); 
    t = strtok(NULL, " \n"); 
    nVar = atoi(t);
    t = strtok(NULL, " \n"); 
    nClause = atoi(t) + 2 * nKey;
    fprintf(fWrite, "p cnf %i %i\n", nVar, nClause);
    for(int i=0; i<nKey; i++) // key assumptions
        fprintf(fWrite, "%i 0\n", Vec_IntEntry(pOut1Key, i)+1);
    for(int i=0; i<nKey; i++)
        fprintf(fWrite, "%i 0\n", Vec_IntEntry(pOut2Key, i)+1);
    while(fgets(buff, buffL, fRead)) {
        fprintf(fWrite, "%s", buff);
    }
    fclose(fRead);
    fclose(fWrite);
    remove(cnfFileName);
    rename(tmpFileName, cnfFileName);

    // clean up
    sat_solver_delete(pSolver);
    Abc_NtkDelete(pNtkOut1);
    Abc_NtkDelete(pNtkOut2);
    Vec_IntFree(pAlphaControls);
    Vec_IntFree(pGammaControls);
    Vec_IntFree(pOut1Pi); Vec_IntFree(pOut1Key); Vec_IntFree(pOut1Po);
    Vec_IntFree(pOut2Pi); Vec_IntFree(pOut2Key); Vec_IntFree(pOut2Po);
}

// Construct general miter network of the key inserted pNtk 
Abc_Ntk_t * Sample_MC_MiterInt(Abc_Ntk_t * pNtk, int nKey) {
    char buf[1000];
    Abc_Ntk_t * pNtkDup, *pNtkMiter;
    int nPi = Abc_NtkCiNum(pNtk) - nKey;

    assert( Abc_NtkIsStrash(pNtk) );

    // Start a new network
    pNtkMiter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    sprintf( buf, "%s_key_inserted_miter", pNtk->pName);
    pNtkMiter->pName = Extra_UtilStrsav(buf);

    // Perform strashing
    pNtkDup = Abc_NtkDup(pNtk);
    ntkMiterPrepare( pNtk, pNtkDup, pNtkMiter, nPi, nKey);
    ntkMiterAddOne( pNtk, pNtkMiter);
    ntkMiterAddOne( pNtkDup, pNtkMiter);
    ntkMiterFinalize( pNtk, pNtkDup, pNtkMiter);
    Abc_AigCleanup((Abc_Aig_t *)pNtkMiter->pManFunc);

    if ( !Abc_NtkCheck( pNtkMiter ) )
    {
        printf( "Sample_MC_MiterInt: The network check has failed.\n" );
        Abc_NtkDelete( pNtkMiter );
        return NULL;
    }

// Debug
//    Abc_Obj_t * pObj; 
//    int i;
//    
//    Abc_NtkPrintStats(pNtkMiter, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
//    Abc_NtkForEachCi( pNtkMiter, pObj, i)
//        printf(" %s", Abc_ObjName(pObj));
//    printf("\n");
//    Abc_NtkForEachCo( pNtkMiter, pObj, i)
//        printf(" %s", Abc_ObjName(pObj));
//    printf("\n");

    return pNtkMiter;
}

// Prepares the network for mitering 
void ntkMiterPrepare( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter, int nPi, int nKey)
{
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    char buf[1000];

    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkMiter);
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkMiter);

    // create new PIs and remember them in the old PIs. Key inputs stay as two copies
    for(int i=0; i<nPi; i++) {
        pObj = Abc_NtkCi(pNtk1, i);
        pObjNew = Abc_NtkCreatePi( pNtkMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        pObj = Abc_NtkCi(pNtk2, i);  
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    for(int i=nPi; i<nPi+nKey; i++) {
        pObj = Abc_NtkCi(pNtk1, i);
        pObjNew = Abc_NtkCreatePi( pNtkMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        sprintf( buf, "%s_1", Abc_ObjName(pObj));
        Abc_ObjAssignName( pObjNew, buf, NULL );
    }
    for(int i=nPi; i<nPi+nKey; i++) {
        pObj = Abc_NtkCi(pNtk2, i);
        pObjNew = Abc_NtkCreatePi( pNtkMiter );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        sprintf( buf, "%s_2", Abc_ObjName(pObj));
        Abc_ObjAssignName( pObjNew, buf, NULL );
    }
    // create POs
    Abc_NtkForEachCo( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePo( pNtkMiter );
        sprintf( buf, "miter_%s", Abc_ObjName(pObj));
        Abc_ObjAssignName( pObjNew, buf, NULL);
    }  
}

// Performs mitering for one network
void ntkMiterAddOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter) {
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    Abc_AigForEachAnd( pNtk, pNode, i )
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
}

// Finalize the miter by adding the output part
void ntkMiterFinalize( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter) {
    Abc_Obj_t * pMiter, * pNode;
    int i;

    // Add miter for all Po
    Abc_NtkForEachCo(pNtk1, pNode, i ) {
        pMiter = Abc_AigXor( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild0Copy(Abc_NtkCo(pNtk2, i)));
        Abc_ObjAddFanin( Abc_NtkPo(pNtkMiter, i), pMiter);
    }
}

void Write_File_Miter_Counting( Abc_Ntk_t * pNtkMiter, char * fileName, int nPi, int nKey) {
    sat_solver *pSolver = sat_solver_new();
    int cid;
    int nPo = Abc_NtkCoNum(pNtkMiter);

    Vec_Int_t *pPi = Vec_IntAlloc(nPi);
    Vec_Int_t *pKey1 = Vec_IntAlloc(nKey);
    Vec_Int_t *pKey2 = Vec_IntAlloc(nKey);
    Vec_Int_t *pPo = Vec_IntAlloc(nPo);
    {
        Aig_Man_t *pAig = Abc_NtkToDar(pNtkMiter, 0, 0);
        Cnf_Dat_t *pCnf = Cnf_Derive(pAig, nPo);
        Cnf_DataLift(pCnf, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnf);
        // store critical variables
        for (int i = 0; i < nPi; i++)
        {
            Aig_Obj_t *pAigObj = Aig_ManCi(pCnf->pMan, i);
            int var = pCnf->pVarNums[Aig_ObjId(pAigObj)];
            Vec_IntPush(pPi, var);
        }
        for (int i = nPi; i < nPi+nKey; i++)
        {
            Aig_Obj_t *pAigObj = Aig_ManCi(pCnf->pMan, i);
            int var = pCnf->pVarNums[Aig_ObjId(pAigObj)];
            Vec_IntPush(pKey1, var);
        }
        for (int i = nPi+nKey; i < nPi+nKey+nKey; i++)
        {
            Aig_Obj_t *pAigObj = Aig_ManCi(pCnf->pMan, i);
            int var = pCnf->pVarNums[Aig_ObjId(pAigObj)];
            Vec_IntPush(pKey2, var);
        }
        for (int i = 0; i < nPo; i++)
        {
            Aig_Obj_t *pAigObj = Aig_ManCo(pCnf->pMan, i);
            Aig_Obj_t *pAigObjFanin1 = Aig_ObjFanin0(pAigObj);
            int var = pCnf->pVarNums[Aig_ObjId(pAigObjFanin1)];
            Vec_IntPush(pPo, var);
        }
        // memory free
        Cnf_DataFree(pCnf);
        Aig_ManStop(pAig);
    }
    
    // Assert miter outputs to be at least one bit different
    lit Lits_Gamma[nPo];
    for(int i=0; i<nPo; i++)
    {
        int var = Vec_IntEntry(pPo, i);
        Lits_Gamma[i] = toLitCond(var, 0);
    }
    cid = sat_solver_addclause(pSolver, Lits_Gamma, Lits_Gamma + nPo);
    assert(cid);
    // sat_solver_print(pSolver, 1);

    // Note: option fIncrement adds an 0 to the end of each line, and
    // to accomplish that, every variable index is also incremented. 
    // Thus, when writing directly to the output files ,do not forget 
    // to increment the target variable index.
    Sat_SolverWriteDimacs(pSolver, fileName, NULL, NULL, 1);
    Write_File_Miter_Counting_Header(fileName);
}

// Add header & key variable assertions to file
void Write_File_Miter_Counting_Header(char * fileName) {

}

// Modify key values asserted in the CNF file.
void AddKeyInfo2CNF(char* cnfFileName, int correctKey, int wrongKey, int nKey) {
    // key value assumptions are written to approxmc file starting at line 2, with a total of 2*nKey lines
    // line 0 is the indicated counting set
    // line 1 is the general cnf header

    char *bCorrectKey = int2bitstring(correctKey, nKey);
    char *bWrongKey = int2bitstring(wrongKey, nKey);

    char buff[102400];
    char* t;
    int lineCount = -2;
    FILE* fSrc = fopen(cnfFileName, "r");
    FILE *fTmp = fopen("tmp.dimacs", "w");
    while(fgets(buff, 102400, fSrc)) {
        if(lineCount >= 0 && lineCount < nKey) { // correct key
            t = strtok(buff, " \n");
            fprintf(fTmp, "%s %c 0\n", t, (char)bCorrectKey[lineCount]);
        }
        else if(lineCount >= nKey && lineCount < 2*nKey) { // wrong key
            t = strtok(buff, " \n");
            fprintf(fTmp, "%s %c 0\n", t, (char)bWrongKey[lineCount - nKey]);
        }
        else {
            fprintf(fTmp, "%s", buff);
        }
        lineCount++;
    }

    fclose(fSrc);
    fclose(fTmp);

    remove(cnfFileName);
    rename("tmp.dimacs", cnfFileName);   
}

////////////////////////////////////////////////////////////////////////
///                        AUXILIARY                                 ///
////////////////////////////////////////////////////////////////////////
char * int2bitstring(int value, int length) {
    char * one_line = new char[length+1];
    one_line[length] = '\0';
    for(int i=0; i<length; i++) {
        int rbit = value%2;
        one_line[i] = (rbit)? '1': '0';
        value = value >> 1;
    }
    return one_line;
}

void write_clause_to_file(FILE* ff, int& nClause, lit* begin, lit* end)
{
    if(ff == NULL)
        return;

    lit* i;
    nClause++;
    for(i=begin; i<end; i++)
        fprintf(ff, "%s%d ", (*i)&1 ? "-":"", (*i)>>1 );
    fprintf(ff, "0\n");
}

// ((var1 == var2) + varCond)
int sat_solver_conditional_unequal(FILE* ff, int& nClause,  sat_solver * pSat, int iVar, int iVar2, int iVarCond )
{
    lit Lits[3];
    int Cid;
    assert( iVar >= 0 );
    assert( iVar2 >= 0 );
    assert( iVarCond >= 0 );

    Lits[0] = toLitCond( iVarCond, 0 );
    Lits[1] = toLitCond( iVar, 1 );
    Lits[2] = toLitCond( iVar2, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert( Cid );
    
    Lits[0] = toLitCond( iVarCond, 0 );
    Lits[1] = toLitCond( iVar, 0 );
    Lits[2] = toLitCond( iVar2, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert( Cid );
    
    return 2;
}

// ((var1 != var2) <-> varCond)
int sat_solver_iff_unequal(FILE* ff, int& nClause, sat_solver *pSat, int iVar, int iVar2, int iVarCond)
{
    // (x!=x' <> cond)
    // (x=x' + cond)^(x!=x + ~cond)
    lit Lits[3];
    int Cid;
    assert(iVar >= 0);
    assert(iVar2 >= 0);
    assert(iVarCond >= 0);

    // (x=x' + cond)
    Lits[0] = toLitCond(iVarCond, 0);
    Lits[1] = toLitCond(iVar, 1);
    Lits[2] = toLitCond(iVar2, 0);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert(Cid);

    Lits[0] = toLitCond(iVarCond, 0);
    Lits[1] = toLitCond(iVar, 0);
    Lits[2] = toLitCond(iVar2, 1);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert(Cid);

    // (x!=x + ~cond)
    Lits[0] = toLitCond(iVarCond, 1);
    Lits[1] = toLitCond(iVar, 0);
    Lits[2] = toLitCond(iVar2, 0);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert(Cid);

    Lits[0] = toLitCond(iVarCond, 1);
    Lits[1] = toLitCond(iVar, 1);
    Lits[2] = toLitCond(iVar2, 1);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert(Cid);

    return 4;
}

int sat_solver_and(FILE* ff, int& nClause, sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    write_clause_to_file(ff, nClause, Lits, Lits + 2);
    assert( Cid );

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    write_clause_to_file(ff, nClause, Lits, Lits + 2);
    assert( Cid );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    write_clause_to_file(ff, nClause, Lits, Lits + 3);
    assert( Cid );
    return 3;
}

int sat_solver_buffer(FILE* ff, int& nClause, sat_solver * pSat, int iVarA, int iVarB, int fCompl )
{
    lit Lits[2];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, !fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    if ( Cid == 0 )
        return 0;
    write_clause_to_file(ff, nClause, Lits, Lits + 2);
    assert( Cid );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    if ( Cid == 0 )
        return 0;
    write_clause_to_file(ff, nClause, Lits, Lits + 2);
    assert( Cid );
    return 2;
}

int sat_solver_const(FILE* ff, int& nClause, sat_solver * pSat, int iVar, int fCompl )
{
    lit Lits[1];
    int Cid;
    assert( iVar >= 0 );

    Lits[0] = toLitCond( iVar, fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 1 );
    write_clause_to_file(ff, nClause, Lits, Lits + 1);
    assert( Cid );
    return 1;
}

int sat_solver_addclause_from( sat_solver* pSat, Cnf_Dat_t * pCnf )
{
    for (int i = 0; i < pCnf->nClauses; i++ )
	{
		if (!sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ))
            return false;
	}
    return true;
}

void sat_solver_print( sat_solver* pSat, int fDimacs )
{
    Sat_Mem_t * pMem = &pSat->Mem;
    clause * c;
    int i, k, nUnits;

    // count the number of unit clauses
    nUnits = 0;
    for ( i = 0; i < pSat->size; i++ )
        if ( pSat->levels[i] == 0 && pSat->assigns[i] != 3 )
            nUnits++;

//    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    printf( "p cnf %d %d\n", pSat->size, Sat_MemEntryNum(&pSat->Mem, 0)-1+Sat_MemEntryNum(&pSat->Mem, 1)+nUnits );

    // write the original clauses
    Sat_MemForEachClause( pMem, c, i, k )
    {
        int i;
        for ( i = 0; i < (int)c->size; i++ )
            printf( "%s%d ", (lit_sign(c->lits[i])? "-": ""),  lit_var(c->lits[i]) + (fDimacs>0) );
        if ( fDimacs )
            printf( "0" );
        printf( "\n" );
    }

    // write the learned clauses
//    Sat_MemForEachLearned( pMem, c, i, k )
//        Sat_SolverClauseWriteDimacs( pFile, c, fDimacs );

    // write zero-level assertions
    for ( i = 0; i < pSat->size; i++ )
        if ( pSat->levels[i] == 0 && pSat->assigns[i] != 3 ) // varX
            printf( "%s%d%s\n",
                     (pSat->assigns[i] == 1)? "-": "",    // var0
                     i + (int)(fDimacs>0),
                     (fDimacs) ? " 0" : "");

    printf( "\n" );

}


ABC_NAMESPACE_IMPL_END