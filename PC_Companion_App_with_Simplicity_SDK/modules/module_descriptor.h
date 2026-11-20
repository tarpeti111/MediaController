#ifndef MODULE_DESCRIPTOR_H
#define MODULE_DESCRIPTOR_H

#include "../app_builder.h"

/*
 * Single source of truth for a module: metadata shown in the GUI and the
 * callbacks used by the model. Descriptors are const and never modified at
 * runtime, so the GUI thread may read them without locking.
 */
typedef struct {
    const char *id;
    const char *name;
    const char *version;
    const char *description;
    const char *icon;      /* Path to a PNG for the GUI, or NULL */
    const char *image;     /* 32x32 bitstring sent to the device */

    app_on_app_launch_t on_launch;
    app_on_input_t on_input;
    app_on_app_process_action_t on_process_action;
    app_on_playlist_request_t on_playlist_request;

    void (*init)(void);
} module_descriptor_t;

#endif
