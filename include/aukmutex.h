#ifndef AUKMUTEX_H
#define AUKMUTEX_H

/*
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "compilers.h"
/* #include <proto/graphics.h> */  /* Temporarily commented - not in AmigaStack */
typedef struct AukMutex AukMutex;

/* Base object vtable - all objects must implement these */
struct AukMutex {
    signed short n,m;
};


#include <proto/exec.h>
/* simple quick version
*/
INLINE void aukMutex_lock(AukMutex *m)
{
    while(m->m>0)
    {
        WaitTOF();
    }
    m->m++;
    while(m->n>0)
    {
        WaitTOF();
    }
    m->n++;
    m->m--;
}
INLINE void aukMutex_unlock(AukMutex *m)
{
   if(m->n==0)
   {
    return;
   }
   m->n--;
}


#ifdef __cplusplus
}
#endif

#endif /* AUKMUTEX_H */
