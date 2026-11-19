#include "modules.h"

#include "desktop/desktop.h"
#include "spotify/spotify.h"

#include "../model.h"
#include "../gui/gui_modules.h"
#include <glib.h>

static const module_descriptor_t *module_registry[] = {
    &desktop_module,
    &spotify_module
};

void modules_init(void)
{
    size_t count = sizeof(module_registry) / sizeof(module_registry[0]);

    if (!gui_modules_init("modules")) {
        g_warning("Some module metadata could not be loaded");
    }

    for (size_t i = 0; i < count; i++) {
        const module_descriptor_t *module = module_registry[i];

        if (module->init != NULL)
            module->init();

        model_add_app(
            module->id,
            module->name,
            module->image,
            module->on_launch,
            module->on_input,
            module->on_process_action
        );
    }
}