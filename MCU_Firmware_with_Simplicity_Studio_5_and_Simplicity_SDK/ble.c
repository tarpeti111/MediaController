/*
 * ble.c
 *
 *  Created on: Mar 30, 2026
 *      Author: tarpe
 */




#include <ble.h>
#include <sl_bgapi.h>
#include "sl_bt_api.h"
#include "model.h"
#include "gatt_db.h"
#include "model.h"
#include "app_assert.h"

uint8_t connection_handle = SL_BT_INVALID_CONNECTION_HANDLE;

uint32_t app_info_service_handle = 0;
uint32_t app_info_data_handle = 0;
uint32_t app_info_choice_handle = 0;

uint8_t app_info_service_uuid[] = {
    0xfe, 0x27, 0xb0, 0x65, 0xcf, 0xa2, 0xcb, 0x80,
    0x3d, 0x4e, 0xcd, 0x32, 0x0e, 0xcb, 0xbd, 0x03
};
uint8_t app_info_data_uuid[] = {
    0xfb, 0xc8, 0xd9, 0xdf, 0xe5, 0xf3, 0xb5, 0xa0,
    0xc6, 0x42, 0xe6, 0x99, 0x7f, 0x7c, 0x6c, 0xc9
};
uint8_t app_info_choice_uuid[] = {
    0x4c, 0xd6, 0xc0, 0xb4, 0x70, 0xf0, 0xdb, 0x8d,
    0x5f, 0x44, 0xaf, 0x86, 0x72, 0x5e, 0x1d, 0xaa
};

bool app_info_service_discovered = false;
bool app_info_data_discovered = false;
bool app_info_choice_discovered = false;
bool app_info_data_notifications_enabled = false;

sl_status_t ble_write_app_choice(){
  uint8_t app_id = model_get_app_id_at_cursor();
  return sl_bt_gatt_write_characteristic_value(connection_handle, app_info_choice_handle, 1, &app_id);
}

sl_status_t ble_notify_input_states(uint8_t input_states[gattdb_input_states_len]){
  return sl_bt_gatt_server_notify_all(gattdb_input_states, gattdb_input_states_len, input_states);
}

void ble_connect(){
  sl_status_t sc = sl_bt_scanner_stop();
  app_assert_status(sc);
  bd_addr* addr = model_get_address_at_cursor();
  sc = sl_bt_connection_open(*addr, model_get_address_type_at_cursor(), sl_bt_gap_1m_phy, &connection_handle);
  app_assert_status(sc);
}

void ble_disconnect(){
  sl_status_t sc = sl_bt_connection_close(connection_handle);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_OK;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,
                                          160,  // interval
                                          80);  // window
      app_assert_status(sc);

      // Start scanning on 1M PHY for generic discoverable devices
      sc = sl_bt_scanner_start(sl_bt_gap_1m_phy,
                               sl_bt_scanner_discover_generic);
      app_assert_status(sc);
      model_change_state(state_connection_select);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      sc = sl_bt_gatt_discover_primary_services_by_uuid(connection_handle, sizeof(app_info_service_uuid), app_info_service_uuid);
      model_change_state(state_app_select);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.

    case sl_bt_evt_connection_closed_id:
      app_info_service_discovered = false;
      app_info_data_discovered = false;
      app_info_choice_discovered = false;
      app_info_data_notifications_enabled = false;

      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,
                                                160,  // interval
                                                80);  // window
      app_assert_status(sc);

      // Start scanning on 1M PHY for generic discoverable devices
      sc = sl_bt_scanner_start(sl_bt_gap_1m_phy,
                               sl_bt_scanner_discover_generic);
      app_assert_status(sc);
      model_change_state(state_connection_select);
      model_on_disconnect();
      break;
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      model_add_address(&evt->data.evt_scanner_legacy_advertisement_report);
      break;

    case sl_bt_evt_gatt_service_id:
       app_info_service_handle = evt->data.evt_gatt_service.service;
       app_info_service_discovered = true;
       break;
    case sl_bt_evt_gatt_characteristic_id:
      uint8_t *uuid = evt->data.evt_gatt_characteristic.uuid.data;
      uint8_t len = evt->data.evt_gatt_characteristic.uuid.len;

      if (len == sizeof(app_info_data_uuid) &&
          memcmp(uuid, app_info_data_uuid, len) == 0) {
        app_info_data_handle = evt->data.evt_gatt_characteristic.characteristic;
        app_info_data_discovered = true;
      }
      else if (len == sizeof(app_info_choice_uuid) &&
          memcmp(uuid, app_info_choice_uuid, len) == 0) {
        app_info_choice_handle = evt->data.evt_gatt_characteristic.characteristic;
        app_info_choice_discovered = true;
      }
      break;
    case sl_bt_evt_gatt_procedure_completed_id:
       if(app_info_service_discovered && !app_info_data_discovered){
         sc = sl_bt_gatt_discover_characteristics_by_uuid(connection_handle, app_info_service_handle, sizeof(app_info_data_uuid), app_info_data_uuid);
         app_assert_status(sc);
         app_info_data_discovered = true;
       }
       else if(app_info_data_discovered && !app_info_data_notifications_enabled){
         sc = sl_bt_gatt_set_characteristic_notification(
               connection_handle,
               (uint16_t)app_info_data_handle,
               sl_bt_gatt_notification);
         app_assert_status(sc);
         app_info_data_notifications_enabled = true;
       }
       else if(app_info_service_discovered && !app_info_choice_discovered){
         sc = sl_bt_gatt_discover_characteristics_by_uuid(connection_handle, app_info_service_handle, sizeof(app_info_choice_uuid), app_info_choice_uuid);
         app_assert_status(sc);
         app_info_choice_discovered = true;
       }
       break;
    case sl_bt_evt_gatt_characteristic_value_id:
      uint32_t char_id = evt->data.evt_gatt_characteristic_value.characteristic;
      if(char_id == app_info_data_handle){
         uint8_t* data = evt->data.evt_gatt_characteristic_value.value.data;
         model_add_app(data);
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
