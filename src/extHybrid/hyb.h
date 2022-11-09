#ifndef ABC__ext__hyb_h
#define ABC__ext__hyb_h

#include "base/abc/abc.h"
#include "opt/cut/cut.h"

ABC_NAMESPACE_HEADER_START

typedef struct Hyb_Man_t_ Hyb_Man_t;

struct Hyb_Man_t_
{
    // runtime statistics
};

// hybMan.c
extern Hyb_Man_t *      Hyb_ManStart();
extern void             Hyb_ManStop( Hyb_Man_t * p );
extern void             Hyb_ManPrintStats ( Hyb_Man_t * p );

// hybEva.c
extern int              Hyb_PoHybridize( Hyb_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pPo );


ABC_NAMESPACE_HEADER_END
#endif