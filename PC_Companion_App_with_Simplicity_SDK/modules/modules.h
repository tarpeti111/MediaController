#ifndef MODULES_H
#define MODULES_H

#include <stddef.h>

#include "module_descriptor.h"

/* Runs each module's init() and registers it with the model.
 * Call from the Silicon Labs thread. */
void modules_init(void);

/* Read-only access to the compiled-in module registry.
 * Safe to call from any thread. */
size_t modules_count(void);
const module_descriptor_t *modules_get(size_t index);

#endif
