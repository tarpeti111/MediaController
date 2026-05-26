/*
 * model.c
 *
 *  Created on: Mar 30, 2026
 *      Author: tarpe
 */



#include "model.h"
#include "stdbool.h"
#include "stdio.h"
#include "display.h"
#include "ble.h"
#include "sl_simple_led_instances.h"

#define MAX_DEVICES 32
#define MAX_NAME_LEN 20
#define MAX_APPS 12

typedef struct {
  app_t apps[MAX_APPS];
  uint8_t app_count;

  bd_addr addresses[MAX_DEVICES];
  uint8_t address_types[MAX_DEVICES];
  char address_names[MAX_DEVICES][20];
  uint8_t address_count;

  uint8_t cursor_position;
  uint8_t input_states[2];

  state_t state;
}app_model_t;

static app_model_t app_model;

//FLAGS
bool app_added;
bool address_added;
bool cursor_moved = false;
bool input_evt = false;
bool mac_adress_view = false;
bool app_ui_drawn = false;

void model_init(){
  app_model.state = state_init;
  app_model.cursor_position = 0;
  app_model.address_count = 0;
}

void model_process_action(){
  switch (app_model.state){
    case state_init:
      break;
    case state_connection_select:
      if(cursor_moved || address_added){
          //Reset Flags
          cursor_moved = false;
          address_added = false;

          //Display addresses
          display_clear();
          if(mac_adress_view){
              display_draw_addresses(app_model.addresses, app_model.address_count, app_model.cursor_position);
          }
          else{
              display_draw_address_names(app_model.address_names, app_model.address_count, app_model.cursor_position);
          }
          display_draw_connection_ui();
          display_update();
      }
      else if(app_model.address_count <= 0){
          display_clear();
          display_no_address_found();
          display_update();
      }
      break;
    case state_app_select:
      if(cursor_moved || app_added){
          //Reset Flags
          cursor_moved = false;
          app_added = false;
          display_clear();
          display_draw_apps(app_model.apps, app_model.app_count, app_model.cursor_position);
          display_update();
      }
      else if(app_model.app_count <= 0){
          display_clear();
          display_no_apps_found();
          display_update();
      }
      break;
    case state_app_running:
      if(input_evt){
          input_evt = false;
          ble_notify_input_states(app_model.input_states);
      }
      if(!app_ui_drawn){
          app_ui_drawn = true;
          display_clear();
          display_draw_app_ui();
          display_update();
      }
      break;
    default:
      break;
  }
}

static void extract_name(char *remote_name, const uint8_t *pbuf, uint8_t buf_len)
{
  uint8_t adv_len;
  uint8_t adv_type;
  uint8_t ix;
  uint8_t name_len = 0;

  ix = 0;

  while (ix < buf_len) {
    adv_len = pbuf[ix];
    ix += sizeof(uint8_t);
    adv_type = pbuf[ix];

    switch (adv_type) {
      case 0x08:
      case 0x09: {
        name_len = adv_len - 1;
        memcpy(remote_name, &pbuf[ix + 1], name_len);
        remote_name[name_len] = 0;
        return;
      }
        // no break
      default:
        break;
    }

    ix += adv_len;
  }
  if(name_len == 0){
      sprintf(remote_name, "NAME N/A");
  }
}

void model_add_app(uint8_t* data){
  memcpy(&app_model.apps[app_model.app_count], data, sizeof(app_model.apps[0]));
  app_model.app_count++;
  app_added = true;
}

void model_add_address(sl_bt_evt_scanner_legacy_advertisement_report_t *report){
  bd_addr new_addr = report->address;
  uint8_t new_addr_type = report->address_type;

  bool is_in_array = false;

  for (int i = 0; i < app_model.address_count; i++) {
    if (memcmp(&app_model.addresses[i], &new_addr, sizeof(bd_addr)) == 0) {
      is_in_array = true;
      break;
    }
  }
  if (!is_in_array && app_model.address_count < MAX_DEVICES) {
      app_model.addresses[app_model.address_count] = new_addr;
      app_model.address_types[app_model.address_count] = new_addr_type;
    extract_name(app_model.address_names[app_model.address_count], report->data.data, report->data.len);
    app_model.address_count++;
  }
  address_added = true;
}

void model_move_cursor(bool up) {
    int count = 0;

    if (app_model.state == state_connection_select) {
        count = app_model.address_count;
    }
    else if (app_model.state == state_app_select) {
        count = app_model.app_count;
    }

    if (count <= 0) return;

    if (up) {
        app_model.cursor_position = (app_model.cursor_position == 0) ? count - 1 : app_model.cursor_position - 1;
    }
    else {
        app_model.cursor_position = (app_model.cursor_position >= count - 1) ? 0 : app_model.cursor_position + 1;
    }

    cursor_moved = true;
}

void model_input_update(uint8_t input[gattdb_input_states_len]){
  for(size_t i = 0; i < gattdb_input_states_len; i++){
      app_model.input_states[i] = input[i];
  }

  input_evt = true;
}

void model_on_disconnect(){
  app_model.address_count = 0;
  app_model.app_count = 0;
  app_ui_drawn = false;
}

void model_on_btn_select(){
  switch (app_model.state) {
    case state_connection_select:
      if(app_model.cursor_position < app_model.address_count){
          ble_connect();
          app_model.state = state_app_select;
      }
      app_model.cursor_position = 0;
      break;
    case state_app_select:
      if(app_model.cursor_position <= app_model.app_count){
          ble_write_app_choice();
          app_model.state = state_app_running;
      }
      app_model.cursor_position = 0;
      break;
    case state_app_running:
        ble_disconnect();
      break;
    default:
      break;
  }
}

void model_on_btn_start(){
  switch (app_model.state) {
        case state_connection_select:
          app_model.cursor_position = 0;
          app_model.address_count = 0;
          break;
        case state_app_select:
          app_model.cursor_position = 0;
          break;
        case state_app_running:
          break;
        default:
          break;
      }
}

uint8_t model_get_app_id_at_cursor(){
  return app_model.apps[app_model.cursor_position].id;
}

bd_addr* model_get_address_at_cursor(){
  return &app_model.addresses[app_model.cursor_position];
}

uint8_t model_get_address_type_at_cursor(){
  return app_model.address_types[app_model.cursor_position];
}

uint8_t model_get_cursor_position(){
  return app_model.cursor_position;
}

void model_set_cursor_position(uint8_t pos){
  app_model.cursor_position = pos;
}

state_t model_get_state(){
  return app_model.state;
}

void model_change_state(state_t s){
  app_model.state = s;
}

void model_change_address_view(){
  mac_adress_view = !mac_adress_view;
}
