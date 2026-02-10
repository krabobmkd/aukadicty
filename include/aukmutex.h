#ifndef AUKMUTEX_H
#define AUKMUTEX_H

/*
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "compilers.h"
#ifdef AMIGA
#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/semaphores.h>
#endif
#include "aukdefs.h"

#ifdef AMIGA
struct AukMutex {
    struct SignalSemaphore *semaphore;
};

INLINE void aukMutex_init(AukMutex *m)
{
    m->semaphore = AllocVec(sizeof(struct SignalSemaphore),MEMF_CLEAR|MEMF_PUBLIC);
    if(!m->semaphore) return;
    InitSemaphore(m->semaphore);
}

INLINE void aukMutex_lock(AukMutex *m)
{

}
INLINE void aukMutex_unlock(AukMutex *m)
{
   if(m->n==0)
   {
    return;
   }
   m->n--;
}
INLINE void aukMutex_close(AukMutex *m)
{


}

#else
struct AukMutex {
    signed short n,m;
};

#endif

/*  */




#ifdef __cplusplus
}
#endif

#endif /* AUKMUTEX_H */
