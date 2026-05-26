/*
 * input.c
 *
 *  Created on: Mar 30, 2026
 *      Author: tarpe
 */



#include "input.h"
#include "sl_simple_button_instances.h"
#include "stdbool.h"
#include "gatt_db.h"
#include "sl_joystick.h"
#include "model.h"
#include "sl_simple_led_instances.h"

#define JOY_BYTE 0
#define BTN_BYTE 1

#define BTN_START 0x01
#define BTN_SELECT 0x02
#define JOY_C 0x04
#define JOY_E 0x01
#define JOY_S 0x02
#define JOY_W 0x04
#define JOY_N 0x08
#define JOY_NE 0x10
#define JOY_SE 0x20
#define JOY_NW 0x40
#define JOY_SW 0x80

volatile bool btn_start_press = false;
volatile bool btn_select_press = false;

sl_joystick_position_t prev_joy_pos = JOYSTICK_NONE;
sl_joystick_t sl_joystick_handle = JOYSTICK_HANDLE_DEFAULT;

uint8_t input_states[gattdb_input_states_len];

void sl_button_on_change(const sl_button_t *handle)
{
  if (handle == &sl_button_btn_start && sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    btn_start_press = true;
  }
  else if (handle == &sl_button_btn_select && sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    btn_select_press = true;
  }
}

void input_init(){
  sl_joystick_init(&sl_joystick_handle);
  sl_joystick_start(&sl_joystick_handle);
}

void input_process_action(){
  //JOYSTICK
  sl_joystick_position_t joy_pos;
  sl_joystick_get_position(&sl_joystick_handle, &joy_pos);
  if(sl_joystick_handle.state == SL_JOYSTICK_ENABLED && joy_pos != prev_joy_pos){
    prev_joy_pos = joy_pos;
    switch (joy_pos) {
      case JOYSTICK_NONE:
        break;
      case JOYSTICK_C:
        input_states[BTN_BYTE] |= JOY_C;
        if(model_get_state() == state_connection_select){
            model_change_address_view();
        }
        break;
      case JOYSTICK_N:
        input_states[JOY_BYTE] |= JOY_N;
         model_move_cursor(true);
        break;
      case JOYSTICK_E:
        input_states[JOY_BYTE] |= JOY_E;
        break;
      case JOYSTICK_S:
        input_states[JOY_BYTE] |= JOY_S;
         model_move_cursor(false);
        break;
      case JOYSTICK_W:
        input_states[JOY_BYTE] |= JOY_W;
        break;
    }
  }

  //BUTTON
  if(btn_select_press){
      btn_select_press = false;
      input_states[BTN_BYTE] |= BTN_SELECT;
      model_on_btn_select();
  }
  if(btn_start_press){
      btn_start_press = false;
      input_states[BTN_BYTE] |= BTN_START;
      model_on_btn_start();
  }

  //ALL
  if (input_states[0] != 0 || input_states[1] != 0) {
      model_input_update(input_states);
      input_states[0] = 0;
      input_states[1] = 0;
  }
}
