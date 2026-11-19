#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <stdbool.h>

/*
 * Installs a module from a ZIP archive.
 *
 * On failure, error_message receives an allocated error string.
 * The caller must free it with g_free().
 */
bool module_manager_install(
    const char *zip_path,
    char **error_message
);

/*
 * Removes an installed module by its ID.
 *
 * On failure, error_message receives an allocated error string.
 * The caller must free it with g_free().
 */
bool module_manager_remove(
    const char *module_id,
    char **error_message
);

#endif