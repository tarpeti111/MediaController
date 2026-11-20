#include "modules.h"

#include "desktop/desktop.h"
#include "spotify/spotify.h"

#include "../model.h"
#include <glib.h>

/* Add new modules here. */
static const module_descriptor_t *const module_registry[] = {
    &desktop_module,
    &spotify_module
};

#define MODULE_COUNT G_N_ELEMENTS(module_registry)

void modules_init(void)
{
    for (size_t i = 0; i < MODULE_COUNT; i++) {
        const module_descriptor_t *module = module_registry[i];

        if (module->init != NULL)
            module->init();

        if (!model_add_app(
                module->name,
                module->image,
                module->on_launch,
                module->on_input,
                module->on_process_action,
                module->on_playlist_request)) {
            g_warning("Could not register module: %s", module->id);
        }
    }
}

size_t modules_count(void)
{
    return MODULE_COUNT;
}

const module_descriptor_t *modules_get(size_t index)
{
    if (index >= MODULE_COUNT)
        return NULL;

    return module_registry[index];
}
