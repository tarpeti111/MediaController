#include "app_builder.h"
#include "stdbool.h"

void model_init();
void model_process_action();

bool model_add_app(const char* name, const char* bitstring_img, app_on_app_launch_t on_init_cb, app_on_input_t on_input_cb, app_on_app_process_action_t on_app_process_action);
