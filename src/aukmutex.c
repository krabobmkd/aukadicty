#include "aukmutex.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef AMIGA

#include <proto/exec.h>

void aukMutex_init(AukMutex *m)
{
    if( !m || m->semaphore ) return; // already inited

    m->semaphore = (struct SignalSemaphore *)AllocVec(sizeof(struct SignalSemaphore),MEMF_CLEAR|MEMF_PUBLIC);
    if(!m->semaphore) return;
    InitSemaphore(m->semaphore);
    m->n = 0;
    m->inited = 1;
}

void aukMutex_lock(AukMutex *m)
{
    if(!m) return;

    if(m->n != 0) return;

    m->n++;

    if(!m->semaphore) // shouldnt happen
    {
          //  printf(" !!!! aukMutex_lock NOT INITED\n");
         //   exit(0);
         return;
    }

    ObtainSemaphore(m->semaphore);

   //Forbid();
}
void aukMutex_unlock(AukMutex *m)
{
    if(!m) return;
    if(m->n != 1)
    {
       // printf(" !!!! aukMutex_unlock asked when %d\n",m->n);
       // exit(0);
       return;
    }
    m->n--;
   //Permit();
//    /* "Each ObtainSemaphore() call must be balanced
//     * by exactly one ReleaseSemaphore() call." */

    ReleaseSemaphore(m->semaphore);

}
void aukMutex_close(AukMutex *m)
{
    if(m && m->semaphore)
    {
        FreeVec(m->semaphore);
        m->semaphore = NULL;
    }
    m->inited = 0;
}

#else

    void aukMutex_init(AukMutex *m)
    {
        m->n = 0;
        m->inited = 1;
    }
    void aukMutex_lock(AukMutex *m)
    {
        m->n++;

    }
    void aukMutex_unlock(AukMutex *m)
    {
        m->n--;
    }
    void aukMutex_close(AukMutex *m)
    {
        m->inited = 0;
    }

#endif
