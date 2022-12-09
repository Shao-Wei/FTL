#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include "Hybrid.h"

ABC_NAMESPACE_IMPL_START

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"HybridCommand.cc\".\n" );
    return NULL;
}

static int FTL_Hybrid_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int c            = 0;
    int fVerbose     = 0;
    int fInputHybrid = 0;

    Abc_Ntk_t *pNtk, *pNtkRes;
        
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ivh" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'i':
                fInputHybrid ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL)
    {
        printf("Error: Empty network.\n");
    }

    if ( !Abc_NtkIsStrash(pNtk) ) {
        Abc_Ntk_t * pNtkTemp = Abc_NtkStrash( pNtk, 0, 1, 0 );
        Abc_NtkDelete(pNtk);
        pNtk = pNtkTemp;
    }

    // main func - support consecutive IH/ OH later
    pNtkRes = FTL_Hybrid(pNtk, fInputHybrid, fVerbose);

    // return
    // LATER - error in Abc_NtkDelete(), double free? unresolved marks?
    // Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);
    return 0;
    
usage:
    Abc_Print( -2, "usage: ftl_hybrid [-ivh]\n" );
    Abc_Print( -2, "\t              Hybridize network by replacing subcircuit with FTL blocks\n" );
    Abc_Print( -2, "\t-i            : toggle input (output) hybridization [default = %s]\n", (fInputHybrid)? "input": "output");
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;   
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{ 
    Cmd_CommandAdd( pAbc, "FTL_Hybrid", "ftl_hybrid", FTL_Hybrid_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t FTL_Hybrid_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct FTL_Hybrid_registrar
{
    FTL_Hybrid_registrar() 
    {
        Abc_FrameAddInitializer(&FTL_Hybrid_frame_initializer);
    }
} FTL_Hybrid_registrar_;

ABC_NAMESPACE_IMPL_END