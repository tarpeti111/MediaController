#ifndef MODEL_H
#define MODEL_H

#include "app_builder.h"
#include <stdbool.h>

/*
 * The model is owned by the Silicon Labs thread. Do not call these
 * functions from the GUI thread.
 */
void model_init(void);
void model_process_action(void);

bool model_add_app(
    const char *name,
    const char *bitstring_img,
    app_on_app_launch_t on_init_cb,
    app_on_input_t on_input_cb,
    app_on_app_process_action_t on_app_process_action,
    app_on_playlist_request_t on_playlist_request
);

bool model_send_media(const uint8_t* data, uint8_t len);

#endif
