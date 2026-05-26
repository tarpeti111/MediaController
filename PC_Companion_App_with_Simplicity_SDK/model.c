#include "model.h"
#include "stdint.h"
#include "stdbool.h"
#include "sl_status.h"
#include "ble.h"
#include "app_log.h"
#include "string.h"
#include "app_assert.h"

#define MAX_NUMBER_OF_APPS 12

const extended_app_t e_app_default = {
    {"EMPTY",APP_ID_DEFAULT,{0}},
    NULL,NULL,NULL
};

char windows_image[] = "11111111111111111111111111111111\
                        11111111111111111111111111100000\
                        11111111111111111111100000000000\
                        11111111111111100000000000000000\
                        11111100000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        11111111111111111111111111111111\
                        11111111111111111111111111111111\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        00000000000011100000000000000000\
                        11111100000011100000000000000000\
                        11111111111111100000000000000000\
                        11111111111111111111100000000000\
                        11111111111111111111111111100000\
                        11111111111111111111111111111111";

typedef struct {
    extended_app_t e_apps[MAX_NUMBER_OF_APPS];
    uint8_t app_count;
    uint8_t app_choice;
    uint16_t input_states;
}model_t;

static model_t model;

static void send_all_apps(){
  sl_status_t sc;
  for(size_t i = 0; i < MAX_NUMBER_OF_APPS && model.e_apps[i].app.id != APP_ID_DEFAULT; i++){
    sc = ble_send_app_info_data((const uint8_t*)&model.e_apps[i].app);
    app_assert_status(sc);
  }
}

static void input_evt(uint16_t inputs){
    model.input_states = inputs;
    //app_log_info("INPUT STATES CHANGED: %lu" APP_LOG_NL, (long unsigned)model.input_states);
    if(model.app_choice < MAX_NUMBER_OF_APPS && model.e_apps[model.app_choice].app.id != APP_ID_DEFAULT){
        if(model.e_apps[model.app_choice].on_input){
            model.e_apps[model.app_choice].on_input(inputs);
        }
    }
}

static void start_app(uint8_t choice){
    model.app_choice = choice;
    app_log_info("APP SELECTED: %lu" APP_LOG_NL, (long unsigned)model.app_choice);
    if(model.app_choice < MAX_NUMBER_OF_APPS && model.e_apps[model.app_choice].app.id != APP_ID_DEFAULT){
        if(model.e_apps[model.app_choice].on_app_launch){
            model.e_apps[model.app_choice].on_app_launch();
        }
    }
}

static void on_disconnect(){
    model.app_choice = APP_ID_DEFAULT;
}

static void set_callbacks(){
    ble_set_apps_requested_cb(send_all_apps);
    ble_set_input_received_cb(input_evt);
    ble_set_app_chosen_cb(start_app);
    ble_set_disconnected_cb(on_disconnect);
}

bool model_add_app(const char* name, const char* bitstring_img, app_on_app_launch_t on_launch, app_on_input_t on_input, app_on_app_process_action_t on_app_process_action){
    for (size_t i = 0; i < MAX_NUMBER_OF_APPS; i++){
        if(model.e_apps[i].app.id == APP_ID_DEFAULT){
            model.e_apps[i].app.id = model.app_count;
            model.app_count++;
            strncpy(model.e_apps[i].app.name, name, sizeof(model.e_apps->app.name)-1);
            model.e_apps[i].app.name[sizeof(model.e_apps[i].app.name) - 1] = '\0';
            app_builder_bitstringToBytes(bitstring_img, model.e_apps[i].app.bitmap);
            model.e_apps[i].on_app_launch = on_launch;
            model.e_apps[i].on_input = on_input;
            model.e_apps[i].on_app_process_action = on_app_process_action;
            return true;
        }
    }
    return false;
}

void model_init(){
    model.app_choice = APP_ID_DEFAULT;
    for (size_t i = 0; i < MAX_NUMBER_OF_APPS; i++){
        model.e_apps[i] = e_app_default;
    }
    set_callbacks();

    model_add_app("Windows", windows_image, NULL, NULL, NULL);
}

void model_process_action(){
    if(model.app_choice < MAX_NUMBER_OF_APPS && model.e_apps[model.app_choice].app.id != APP_ID_DEFAULT){
        if(model.e_apps[model.app_choice].on_app_process_action){
            model.e_apps[model.app_choice].on_app_process_action();
        }
    }
}