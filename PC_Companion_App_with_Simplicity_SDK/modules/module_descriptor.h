#ifndef MODULE_DESCRIPTOR_H
#define MODULE_DESCRIPTOR_H

#include "../app_builder.h"

typedef struct {
    const char *id;
    const char *metadata_path;
    const char *name;
    const char *image;

    app_on_app_launch_t on_launch;
    app_on_input_t on_input;
    app_on_app_process_action_t on_process_action;

    void (*init)(void);
} module_descriptor_t;

#endif