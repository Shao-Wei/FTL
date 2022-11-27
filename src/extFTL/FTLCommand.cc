#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include <vector>

#include "ER.h"

ABC_NAMESPACE_IMPL_START

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"FTLCommand.cc\".\n" );
    return NULL;
}

static int Sample_MC_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int c            = 0;
    int fVerbose     = 0;
    int nKey         = 5;
    int fLocked      = 0;
    int fGenMiter    = 0;

    char *blifFileName;
    char Command[1000];

    Abc_Ntk_t* pNtk;
        
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mklvh" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'm':
                fGenMiter ^= 1;
                break;
            case 'k':
                nKey = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'l':
                fLocked = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
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
    if ( argc != globalUtilOptind + 1) 
        goto usage;
    blifFileName = argv[globalUtilOptind];
    sprintf( Command, "read %s", blifFileName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", blifFileName);
        return 1;
    }
    pNtk = Abc_FrameReadNtk(pAbc);

    Sample_MC_Miter(pNtk, nKey, fVerbose, fLocked, fGenMiter);
    
    return 0;
    
usage:
    Abc_Print( -2, "usage: sampleMC <blif>\n" );
    Abc_Print( -2, "\t              SAT based error rate estimation test for c432\n" );
    Abc_Print( -2, "\t<blif>        : network to be key inserted and counted\n" );
    Abc_Print( -2, "\t-k <int>      : number of keys inserted [default = %d]\n", nKey );
    Abc_Print( -2, "\t-l <int>      : provide number of bits inserted to the locked circuit [default = %d]\n", nKey );
    Abc_Print( -2, "\t                (the inserted key bits need to be appended to the original primary inputs\n");
    Abc_Print( -2, "\t-m            : generate cnf file of the miter of a random key, no counting is performed [default = %d]\n", fGenMiter);
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;   
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{ 
    Cmd_CommandAdd( pAbc, "FTL", "sampleMC", Sample_MC_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t FTL_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct FTL_registrar
{
    FTL_registrar() 
    {
        Abc_FrameAddInitializer(&FTL_frame_initializer);
    }
} FTL_registrar_;

ABC_NAMESPACE_IMPL_END