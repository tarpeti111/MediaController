
#include "model.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sl_status.h"
#include "ble.h"
#include "app_log.h"
#include "app_assert.h"
#include "gui/gui_backend.h"

#include <glib.h>

#define MAX_NUMBER_OF_APPS 12

static GMutex model_mutex;
static bool model_initialized = false;

const extended_app_t e_app_default = {
    {"EMPTY", APP_ID_DEFAULT, {0}},
    NULL, NULL, NULL, NULL
};

typedef struct {
    extended_app_t app;
    char module_id[MODEL_MODULE_ID_LENGTH];
} model_module_t;

typedef struct {
    model_module_t modules[MAX_NUMBER_OF_APPS];
    uint8_t app_count;
    uint8_t app_choice;
    uint16_t input_states;
} model_t;

static model_t model;

static void send_all_apps(void)
{
    sl_status_t sc;

    for (size_t i = 0;
         i < MAX_NUMBER_OF_APPS &&
         model.modules[i].app.app.id != APP_ID_DEFAULT;
         i++) {

        sc = ble_send_app_info_data(
            (const uint8_t *)&model.modules[i].app.app
        );

        app_assert_status(sc);
    }
}

static void input_evt(uint16_t inputs)
{
    model.input_states = inputs;

    if (model.app_choice < MAX_NUMBER_OF_APPS &&
        model.modules[model.app_choice].app.app.id != APP_ID_DEFAULT) {

        if (model.modules[model.app_choice].app.on_input) {
            model.modules[model.app_choice].app.on_input(inputs);
        }
    }
}

static void start_app(uint8_t choice)
{
    model.app_choice = choice;

    app_log_info(
        "APP SELECTED: %lu" APP_LOG_NL,
        (long unsigned)model.app_choice
    );

    if (model.app_choice < MAX_NUMBER_OF_APPS &&
        model.modules[model.app_choice].app.app.id != APP_ID_DEFAULT) {

        if (model.modules[model.app_choice].app.on_app_launch) {
            model.modules[model.app_choice].app.on_app_launch();
        }
    }
}

static void on_connect(void)
{
    gui_backend_set_connected(TRUE);
}

static void on_disconnect(void)
{
    model.app_choice = APP_ID_DEFAULT;
    gui_backend_set_connected(FALSE);
}

static void set_callbacks(void)
{
    ble_set_connected_cb(on_connect);
    ble_set_disconnected_cb(on_disconnect);
    ble_set_apps_requested_cb(send_all_apps);
    ble_set_input_received_cb(input_evt);
    ble_set_app_chosen_cb(start_app);
}

bool model_add_app(
    const char *module_id,
    const char *name,
    const char *bitstring_img,
    app_on_app_launch_t on_launch,
    app_on_input_t on_input,
    app_on_app_process_action_t on_app_process_action)
{
    if (module_id == NULL || name == NULL || bitstring_img == NULL)
        return false;

    bool added = false;

    g_mutex_lock(&model_mutex);

    for (size_t i = 0; i < MAX_NUMBER_OF_APPS; i++) {

        if (model.modules[i].app.app.id == APP_ID_DEFAULT) {

            model_module_t *module = &model.modules[i];

            module->app.app.id = model.app_count++;

            g_strlcpy(
                module->app.app.name,
                name,
                sizeof(module->app.app.name)
            );

            g_strlcpy(
                module->module_id,
                module_id,
                sizeof(module->module_id)
            );

            app_builder_bitstringToBytes(
                bitstring_img,
                module->app.app.bitmap
            );

            module->app.on_app_launch = on_launch;
            module->app.on_input = on_input;
            module->app.on_app_process_action = on_app_process_action;

            added = true;
            break;
        }
    }

    g_mutex_unlock(&model_mutex);

    if (added)
        gui_backend_notify_modules_changed();

    return added;
}

size_t model_get_apps(
    model_app_info_t *apps,
    size_t capacity)
{
    if (apps == NULL || capacity == 0)
        return 0;

    g_mutex_lock(&model_mutex);

    if (!model_initialized) {
        g_mutex_unlock(&model_mutex);
        return 0;
    }

    size_t count = 0;

    for (size_t i = 0;
         i < MAX_NUMBER_OF_APPS && count < capacity;
         i++) {

        model_module_t *module = &model.modules[i];

        if (module->app.app.id == APP_ID_DEFAULT)
            break;

        apps[count].id = module->app.app.id;

        g_strlcpy(
            apps[count].module_id,
            module->module_id,
            sizeof(apps[count].module_id)
        );

        g_strlcpy(
            apps[count].name,
            module->app.app.name,
            sizeof(apps[count].name)
        );

        count++;
    }

    g_mutex_unlock(&model_mutex);

    return count;
}

void model_init(void)
{
    g_mutex_lock(&model_mutex);

    model.app_count = 0;
    model.app_choice = APP_ID_DEFAULT;
    model.input_states = 0;

    for (size_t i = 0; i < MAX_NUMBER_OF_APPS; i++) {
        model.modules[i].app = e_app_default;
        memset(model.modules[i].module_id, 0,
               sizeof(model.modules[i].module_id));
    }

    model_initialized = true;

    g_mutex_unlock(&model_mutex);

    set_callbacks();
}

void model_process_action(void)
{
    if (model.app_choice < MAX_NUMBER_OF_APPS &&
        model.modules[model.app_choice].app.app.id != APP_ID_DEFAULT) {

        if (model.modules[model.app_choice].app.on_app_process_action) {
            model.modules[model.app_choice].app.on_app_process_action();
        }
    }
}
