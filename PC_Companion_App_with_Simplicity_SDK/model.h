#ifndef MODEL_H
#define MODEL_H

#include "app_builder.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MODEL_MODULE_ID_LENGTH 32

typedef struct {
    uint8_t id;
    char module_id[MODEL_MODULE_ID_LENGTH];
    char name[16];
} model_app_info_t;

void model_init(void);
void model_process_action(void);

bool model_add_app(
    const char *module_id,
    const char *name,
    const char *bitstring_img,
    app_on_app_launch_t on_init_cb,
    app_on_input_t on_input_cb,
    app_on_app_process_action_t on_app_process_action
);

/* Returns a thread-safe copy of the registered applications. */
size_t model_get_apps(
    model_app_info_t *apps,
    size_t capacity
);

#endif
