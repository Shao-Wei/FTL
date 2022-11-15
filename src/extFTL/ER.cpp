#include "ER.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include <math.h>
#include <vector>
#include <stdlib.h>        // rand()
#include <numeric>         // mean, variance

/*== base/abci/abcDar.c ==*/
extern "C" { Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters); }

ABC_NAMESPACE_IMPL_START
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
void insertKey(Abc_Ntk_t* pNtk, int nKey, int seed, int& correctKey);

// void Sample_MCInt(Abc_Ntk_t * pNtk, int nPi, int nKey, int nPo, char* cnfFileName);
Abc_Ntk_t * Sample_MC_MiterInt(Abc_Ntk_t * pNtk, int nKey);
static void ntkMiterPrepare( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter, int nPi, int nKey);
static void ntkMiterAddOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter);
static void ntkMiterFinalize( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter);
static int  miter_build_solver( sat_solver * pSolver, int * pVarPi, int * pVarKey, Abc_Ntk_t * pNtkMiter, int nPi, int nKey, int fVerbose);
static void Write_Counting_Header(char * fileName, int * pVarPi, int nPi);
static char * getCountingResult(char * fileName);
// void AddKeyInfo2CNF(char* cnfFileName, int correctKey, int wrongKey, int nKey);

//// Aux ///////////////////////////////////////////////////////////////
char * int2bitstring(int value, int length);

static void sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, int fIncrement );
static void sat_SolverWriteDimacs( sat_solver * p, char * pFileName, lit* assumpBegin, lit* assumpEnd, int incrementVars );
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

  Synopsis    [ Sample output corrupted minterms regarding different wrong
  keys through model counting. The construction of miter is through building
  miter circuit. ]

  Description [Prints out statistics over the output corruption behavior.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sample_MC_Miter(Abc_Ntk_t * pNtk, int nKey, int fVerbose) {
    // int fComb        = 1; // toggles deriving combinational miter
    // int fCheck       = 1; // toggles network check, should always be true
    // int fImplic      = 0; // toggles deriving implication miter
    // int fMulti       = 1; // toggles creating multi-output miter
    // int nPartSize    = 0; // output partition size
    // int fTrans       = 0; // toggle XORing pair-wise POs of the miter
    // int fIgnoreNames = 0; // toggle ignoring names when matching CIs/COs 

    Abc_Ntk_t * pNtkSt, *pNtkMiter;
    int nPi = Abc_NtkCiNum(pNtk);
    int seed = 5;
    int correctKey = 0, wrongKey, numKey = 0, rbit;
    int pLits[nKey + nKey]; // assertion lits
    int cid, systemRet;

    char Command[1000];
    char miterFileName[1000];
    sprintf(miterFileName, "MITER.dimacs");
    char countFileName[1000];
    sprintf(countFileName, "MITER.result");

    ER_Man_t * pERMan = ER_ManStart(nPi, nKey);

    // Insert keys
    insertKey(pNtk, nKey, seed, correctKey);

    // Make sure network is strashed
    pNtkSt = Abc_NtkStrash( pNtk, 0, 1, 0 );

    // Compute the miter
    pNtkMiter = Sample_MC_MiterInt(pNtkSt, nKey);
    if( pNtkMiter == NULL ) {
        Abc_Print( -1, "Miter computation has failed.\n" );
        return;
    }

    // Network to CNF
    sat_solver * pSolver = sat_solver_new();
    int * pVarPi = new int[nPi]; // stores var of Pi
    int * pVarKey = new int[nKey + nKey]; // stores var of keys
    cid = miter_build_solver(pSolver, pVarPi, pVarKey, pNtkMiter, nPi, nKey, fVerbose);
    if(!cid) {
        printf("Sample_MC_Miter: miter to CNF checking failed. Abort.\n");
        return;
    }
    // Iterate through all possible keys
    
    // Assert correct key value
    for(int i=0; i<nKey; i++) // correct key = 0x0
        pLits[i] = Abc_Var2Lit(pVarKey[i], 1);
    for(int k=0; k<(1 << nKey); k++ ) {
        // Assert wrong key value
        wrongKey = k;
        if(wrongKey == correctKey)
            continue;
        for(int i=0; i<nKey; i++) {
            rbit = wrongKey%2;
            pLits[nKey+i] = (rbit)? Abc_Var2Lit(pVarKey[nKey+i], 0): Abc_Var2Lit(pVarKey[nKey+i], 1);
            wrongKey = wrongKey >> 1;
        }
        // Write to approxmc file format
        Write_Counting_Header(miterFileName, pVarPi, nPi);
        sat_SolverWriteDimacs(pSolver, miterFileName, pLits, pLits+nKey+nKey, 1);

        // Execute approxmc
        sprintf( Command, "./approxmc -v 0 %s >> %s", miterFileName, countFileName);
        systemRet = system( Command );
        if(systemRet == -1) {
            printf("Call to approxmc failed. Abort.\n");
        }

        ER_AddCount(pERMan, numKey, getCountingResult(countFileName));
        numKey++;
    }

    // Get stats
// ER_PrintVCount(pERMan);
    ER_GetStats(pERMan);
    ER_PrintStats(pERMan);

    // Clean up
    ER_ManStop(pERMan);
    sat_solver_delete(pSolver);
    delete [] pVarPi;
    delete [] pVarKey;
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

    Abc_NtkPrintStats(pNtkMiter, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

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

// Push clauses of the miter into sat solver
int miter_build_solver( sat_solver * pSolver, int * pVarPi, int * pVarKey, Abc_Ntk_t * pNtkMiter, int nPi, int nKey, int fVerbose) {
    int status;
    int cid;
    int nPo = Abc_NtkCoNum(pNtkMiter);

    // Build miter into pSolver
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
            int var = pCnf->pVarNums[Aig_ObjId(pAigObj)];
            Vec_IntPush(pPo, var);
        }
        // memory free
        Cnf_DataFree(pCnf);
        Aig_ManStop(pAig);
    }
    
    // Assert miter outputs to be at least one bit different
    int pMiterLits[nPo];
    for(int i=0; i<nPo; i++) {
        pMiterLits[i] = Abc_Var2Lit(Vec_IntEntry(pPo, i), 0);
    }
    cid = sat_solver_addclause(pSolver, pMiterLits, pMiterLits + nPo);
    assert(cid);

    // Record pVarPi, pVarKey
    for(int i=0; i<nPi; i++)
        pVarPi[i] = Vec_IntEntry(pPi, i);
    for(int i=0; i<nKey; i++)
        pVarKey[i] = Vec_IntEntry(pKey1, i);
    for(int i=0; i<nKey; i++)
        pVarKey[nKey + i] = Vec_IntEntry(pKey2, i);

    /**
     * Equal Key Value Check
     * Assert key1 key2 to be the same
     * SAT solving the miter should be UNSAT
     */
    int pLits[nKey + nKey];
    for(int i=0; i<nKey; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pKey1, i), 0);
    for(int i=0; i<nKey; i++)
        pLits[i+nKey] = Abc_Var2Lit(Vec_IntEntry(pKey2, i), 0);
    status = sat_solver_solve(pSolver, pLits, pLits+nKey+nKey, 0, 0, 0, 0);

    if(fVerbose) {
        printf("Equal Key Value Check: %s\n", (status == l_False)? "pass": "fail");
        if(status == l_True) {
            // print CEX
            Abc_Print( 1, "v" );
            for (int v = 0; v < sat_solver_nvars(pSolver); v++)
            {
                int value = sat_solver_var_value(pSolver, v);
                Abc_Print( 1, " %s%d", (value ? "":"-"), v );
            }
            Abc_Print( 1, "\n" );
        }
    }

    // Clean up
    Vec_IntFree(pPi);
    Vec_IntFree(pKey1);
    Vec_IntFree(pKey2);
    Vec_IntFree(pPo);
    
    return (status == l_False);
}

// Add header to file
void Write_Counting_Header(char * fileName, int * pVarPi, int nPi) {
    FILE* fWrite = fopen(fileName, "w");
    fprintf(fWrite, "c ind"); // c ind <sampled set>
    for(int i=0; i<nPi; i++)
        fprintf(fWrite, " %i", pVarPi[i]+1);
    fprintf(fWrite, " 0\n");
    fclose(fWrite);
}

// Get counting result from approxmc log file
char * getCountingResult(char * fileName) {
    FILE * f = fopen(fileName, "r");
    if(f == NULL) {
        printf("Cannot open counting result file %s. Return 0.\n", fileName);
        return 0;
    }

    static const long max_len = 100; // define the max length of the line to read
    char buff[max_len];
    char *last_newline, *last_line;
    char * t;
    size_t c;
    // Get the last line "s mc <int>"
    fseek(f, -max_len, SEEK_END);
    c = fread(buff, max_len-1, 1, f); // Presumably there can be more than one new line caracter
    fclose(f);

    buff[max_len-1] = '\0'; // Close the string
    last_newline = strrchr(buff, '\n'); // Find last occurrence of newlinw
    last_line = last_newline + 1; // jump to it
    t = strtok(last_line, " \n");
    t = strtok(NULL, " \n"); 
    t = strtok(NULL, " \n"); 

    return t;
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

void sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, int fIncrement )
{
    int i;
    for ( i = 0; i < (int)pC->size; i++ )
        fprintf( pFile, "%s%d ", (lit_sign(pC->lits[i])? "-": ""),  lit_var(pC->lits[i]) + (fIncrement>0) );
    if ( fIncrement )
        fprintf( pFile, "0" );
    fprintf( pFile, "\n" );
}
/***
 * Identical to SAT_SolverWriteDimacs() but with fopen append mode
 * Note: option fIncrement adds an 0 to the end of each line, and
 * to accomplish that, every variable index is also incremented. 
 * Thus, when writing directly to the output files, do not forget 
 * to increment the target variable index.
***/
void sat_SolverWriteDimacs( sat_solver * p, char * pFileName, lit* assumpBegin, lit* assumpEnd, int incrementVars )
{
    Sat_Mem_t * pMem = &p->Mem;
    FILE * pFile;
    clause * c;
    int i, k, nUnits;

    // count the number of unit clauses
    nUnits = 0;
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 )
            nUnits++;

    // start the file
    pFile = pFileName ? fopen( pFileName, "a" ) : stdout;
    if ( pFile == NULL )
    {
        printf( "Sat_SolverWriteDimacs(): Cannot open the ouput file.\n" );
        return;
    }
//    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->size, Sat_MemEntryNum(&p->Mem, 0)-1+Sat_MemEntryNum(&p->Mem, 1)+nUnits+(int)(assumpEnd-assumpBegin) );

    // write the original clauses
    Sat_MemForEachClause( pMem, c, i, k )
        sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write the learned clauses
//    Sat_MemForEachLearned( pMem, c, i, k )
//        Sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write zero-level assertions
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 ) // varX
            fprintf( pFile, "%s%d%s\n",
                     (p->assigns[i] == 1)? "-": "",    // var0
                     i + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");

    // write the assump
    if (assumpBegin) {
        for (; assumpBegin != assumpEnd; assumpBegin++) {
            fprintf( pFile, "%s%d%s\n",
                     lit_sign(*assumpBegin)? "-": "",
                     lit_var(*assumpBegin) + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");
        }
    }

    fprintf( pFile, "\n" );
    if ( pFileName ) fclose( pFile );
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