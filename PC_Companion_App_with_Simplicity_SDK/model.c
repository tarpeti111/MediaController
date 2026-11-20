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

const extended_app_t e_app_default = {
    {"EMPTY", APP_ID_DEFAULT, {0}},
    NULL, NULL, NULL, NULL
};

typedef struct {
    extended_app_t apps[MAX_NUMBER_OF_APPS];
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
         model.apps[i].app.id != APP_ID_DEFAULT;
         i++) {

        sc = ble_send_app_info_data(
            (const uint8_t *)&model.apps[i].app
        );

        app_assert_status(sc);
    }
}

static void input_evt(uint16_t inputs)
{
    model.input_states = inputs;

    if (model.app_choice < MAX_NUMBER_OF_APPS &&
        model.apps[model.app_choice].app.id != APP_ID_DEFAULT) {

        if (model.apps[model.app_choice].on_input) {
            model.apps[model.app_choice].on_input(inputs);
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
        model.apps[model.app_choice].app.id != APP_ID_DEFAULT) {

        if (model.apps[model.app_choice].on_app_launch) {
            model.apps[model.app_choice].on_app_launch();
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

static void model_on_playlist_request(uint16_t request){
    if (model.app_choice < MAX_NUMBER_OF_APPS && model.apps[model.app_choice].app.id != APP_ID_DEFAULT) {
        model.apps[model.app_choice].on_playlist_request(request);
    }
}

static void set_callbacks(void)
{
    ble_set_connected_cb(on_connect);
    ble_set_disconnected_cb(on_disconnect);
    ble_set_apps_requested_cb(send_all_apps);
    ble_set_input_received_cb(input_evt);
    ble_set_app_chosen_cb(start_app);
    ble_set_playlist_request_cb(model_on_playlist_request);
}
bool model_send_media(const uint8_t* data, uint8_t len){
    if(data == NULL || len == 0){
        return false;
    }
    sl_status_t sc;
    sc = ble_send_playlist_data(data, len);
    app_assert_status(sc);
    return true;
}

bool model_add_app(
    const char *name,
    const char *bitstring_img,
    app_on_app_launch_t on_launch,
    app_on_input_t on_input,
    app_on_app_process_action_t on_app_process_action,
    app_on_playlist_request_t on_playlist_request)
{
    if (name == NULL || bitstring_img == NULL)
        return false;

    if (model.app_count >= MAX_NUMBER_OF_APPS)
        return false;

    extended_app_t *app = &model.apps[model.app_count];

    app->app.id = model.app_count++;

    g_strlcpy(app->app.name, name, sizeof(app->app.name));

    app_builder_bitstringToBytes(bitstring_img, app->app.bitmap);

    app->on_app_launch = on_launch;
    app->on_input = on_input;
    app->on_app_process_action = on_app_process_action;
    app->on_playlist_request = on_playlist_request;

    return true;
}

void model_init(void)
{
    model.app_count = 0;
    model.app_choice = APP_ID_DEFAULT;
    model.input_states = 0;

    for (size_t i = 0; i < MAX_NUMBER_OF_APPS; i++) {
        model.apps[i] = e_app_default;
    }

    set_callbacks();
}

void model_process_action(void)
{
    if (model.app_choice < MAX_NUMBER_OF_APPS &&
        model.apps[model.app_choice].app.id != APP_ID_DEFAULT) {

        if (model.apps[model.app_choice].on_app_process_action) {
            model.apps[model.app_choice].on_app_process_action();
        }
    }
}
