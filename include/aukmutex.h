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
#include <stdlib.h>

#ifdef AMIGA
struct AukMutex {
    struct SignalSemaphore *semaphore;
    int n;
};

INLINE void aukMutex_init(AukMutex *m)
{
    if( m->semaphore ) return; // already inited
    m->semaphore = AllocVec(sizeof(struct SignalSemaphore),MEMF_CLEAR|MEMF_PUBLIC);
    if(!m->semaphore) return;
    InitSemaphore(m->semaphore);
    m->n = 0;
}

INLINE void aukMutex_lock(AukMutex *m)
{
    if(m->n != 0)
    {
        printf(" !!!! aukMutex_lock asked when %d\n",m->n);
        exit(0);
    }
    m->n++;
   ObtainSemaphore(m->semaphore);
   //Forbid();
}
INLINE void aukMutex_unlock(AukMutex *m)
{
    if(m->n != 1)
    {
        printf(" !!!! aukMutex_unlock asked when %d\n",m->n);
        exit(0);
    }
    m->n--;
   //Permit();
    /* "Each ObtainSemaphore() call must be balanced
     * by exactly one ReleaseSemaphore() call." */
    ReleaseSemaphore(m->semaphore);
}
INLINE void aukMutex_close(AukMutex *m)
{
    if(m && m->semaphore)
    {
        FreeVec(m->semaphore);
        m->semaphore = NULL;
    }
}

#else
struct AukMutex {
    signed short n,m;
};

INLINE void aukMutex_lock(AukMutex *m)
{
    m->n++;

}
INLINE void aukMutex_unlock(AukMutex *m)
{
    m->n--;

}
#endif

/*  */




#ifdef __cplusplus
}
#endif

#endif /* AUKMUTEX_H */
