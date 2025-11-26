#ifndef EXEC_MEMORY_H
#define EXEC_MEMORY_H

/*
 * AmigaStack - Exec Memory Types
 */

#include <exec/types.h>

/* Memory allocation flags */
#define MEMF_CLEAR (1 << 16)  /* Clear memory after allocation */
#define MEMF_PUBLIC 0         /* Public memory (default) */

#endif /* EXEC_MEMORY_H */
