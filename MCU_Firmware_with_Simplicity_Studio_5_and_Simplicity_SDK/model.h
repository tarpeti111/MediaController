/*
 * model.h
 *
 *  Created on: Mar 30, 2026
 *      Author: tarpe
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <sl_bgapi.h>
#include "sl_bt_api.h"
#include "gatt_db.h"

typedef enum {
  state_init, state_connection_select, state_app_select, state_app_running, state_other
} state_t;

typedef struct __attribute__((packed)){
  char name[16];
  uint8_t id;
  uint8_t bitmap[128];
} app_t;


void model_init();
void model_process_action();

void model_add_address(sl_bt_evt_scanner_legacy_advertisement_report_t *report);
void model_add_app(uint8_t* data);

uint8_t model_get_app_id_at_cursor();
bd_addr* model_get_address_at_cursor();
uint8_t model_get_address_type_at_cursor();

void model_on_disconnect();
void model_on_btn_select();
void model_on_btn_start();

state_t model_get_state();
void model_change_state(state_t s);
void model_change_address_view();

void model_move_cursor(bool up);

uint8_t model_get_cursor_position();

void model_input_update(uint8_t input[gattdb_input_states]);

#endif /* MODEL_H_ */
