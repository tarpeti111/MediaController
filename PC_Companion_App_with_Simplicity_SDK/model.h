#ifndef MODEL_H
#define MODEL_H

#include "app_builder.h"
#include <stdbool.h>
#include <stddef.h>
#include "sl_status.h"

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

/* Sends a media_protocol.h message to the device. Returns the BLE status;
 * fails (without crashing) if the device has not enabled notifications. */
sl_status_t model_send_media(const void *data, size_t len);

#endif
