#ifndef AUKMUTEX_H
#define AUKMUTEX_H

/*
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AMIGA

#include <exec/semaphores.h>
#endif
#include "aukdefs.h"


#ifdef AMIGA
struct AukMutex {
    struct SignalSemaphore *semaphore;
    int n;
    /* some mutex are just never usedand may not be inited (for optimization purpose), test that for mutex that are optionaly inited*/
    int inited;
};
#else
struct AukMutex {
    signed short n;
    signed short inited;
};
#endif

void aukMutex_init(AukMutex *m);
void aukMutex_lock(AukMutex *m);
void aukMutex_unlock(AukMutex *m);
void aukMutex_close(AukMutex *m);


#ifdef __cplusplus
}
#endif

#endif /* AUKMUTEX_H */
