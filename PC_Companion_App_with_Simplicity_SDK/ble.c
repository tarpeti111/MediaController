#include "ble.h"
#include "sli_bt_api.h"
#include "gatt_db.h"
#include "app_assert.h"
#include "app_log.h"
#include "app_log_cli.h"

static ble_connected_cb_t ble_connected_cb = NULL;
static ble_apps_requested_cb_t ble_apps_requested_cb = NULL;
static ble_disconnected_cb_t ble_disconnected_cb = NULL;
static ble_input_received_cb_t ble_input_received_cb = NULL;
static ble_app_chosen_cb_t ble_app_chosen_cb = NULL;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;

uint8_t connection_handle = SL_BT_INVALID_CONNECTION_HANDLE;
uint32_t service_handle;
uint32_t input_characteristic_handle;

static const uint8_t target_service_uuid[] = {
  0x7a, 0x29, 0xd0, 0xec, 0xee, 0x49, 0x68, 0xb4,
  0xad, 0x4d, 0xb6, 0xcd, 0x07, 0xfa, 0x10, 0xaf
};
static const uint8_t target_char_uuid[] = {
  0x0e, 0x1a, 0x05, 0xad, 0x79, 0x5a, 0x1c, 0xab,
  0x36, 0x4a, 0x71, 0x89, 0xb4, 0x74, 0xa5, 0x17
};

bool discovered_characteristics = false;
bool notifications_enabled = false;

void reset_flags(){
  discovered_characteristics = false;
  notifications_enabled = false;
}

sl_status_t ble_send_app_info_data(const uint8_t* data){
  return sl_bt_gatt_server_send_notification(connection_handle, gattdb_app_info_data, gattdb_app_info_data_len, data);
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Extract unique ID from BT Address.
      sc = sl_bt_gap_get_identity_address(&address, &address_type);
      app_assert_status(sc);
      
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to generate advertising data\n",
                 (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);


      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_handle = evt->data.evt_connection_opened.connection;

      if (ble_connected_cb) {
        ble_connected_cb();
      }

      sc = sl_bt_gatt_discover_primary_services_by_uuid(connection_handle, sizeof(target_service_uuid)/sizeof(target_service_uuid[0]), target_service_uuid);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to generate advertising data\n",
                 (int)sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);

      if (ble_disconnected_cb) {
        ble_disconnected_cb();
      }
      
      reset_flags();
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    // -------------------------------
    case sl_bt_evt_gatt_service_id:
      service_handle = evt->data.evt_gatt_service.service;
      break;
    case sl_bt_evt_gatt_characteristic_id:
        input_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      break;
    case sl_bt_evt_gatt_procedure_completed_id:
      if(!discovered_characteristics){
        sc = sl_bt_gatt_discover_characteristics_by_uuid(connection_handle, service_handle, sizeof(target_char_uuid)/sizeof(target_char_uuid[0]), target_char_uuid);
        app_assert_status(sc);
        discovered_characteristics = true;
      }
      else if(discovered_characteristics && !notifications_enabled){
        sc = sl_bt_gatt_set_characteristic_notification(
              connection_handle,
              (uint16_t)input_characteristic_handle,
              sl_bt_gatt_notification);
        app_assert_status(sc);
        notifications_enabled = true;
      }
      break;
    case sl_bt_evt_gatt_characteristic_value_id:
      uint32_t char_id = evt->data.evt_gatt_characteristic_value.characteristic;
      if(char_id == input_characteristic_handle){
        if(ble_input_received_cb){
          uint16_t input_states = (uint16_t)evt->data.evt_gatt_characteristic_value.value.data[0] << 8 | evt->data.evt_gatt_characteristic_value.value.data[1];
          ble_input_received_cb(input_states);
        }
      }
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_app_info_data) {
        if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config
            && evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_notification) {
          if(ble_apps_requested_cb){
            ble_apps_requested_cb();
          }
        }
      }
      break;
    case sl_bt_evt_gatt_server_user_write_request_id:
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_app_choice) {
        if(ble_app_chosen_cb){
          uint8_t app_choice = evt->data.evt_gatt_server_user_write_request.value.data[0];
          ble_app_chosen_cb(app_choice);
        }
        // Send response (MANDATORY)
        sl_bt_gatt_server_send_user_write_response(
          evt->data.evt_gatt_server_user_write_request.connection,
          gattdb_app_choice,
          SL_STATUS_OK
        );
      }
      break;
    // Default event handler.
    default:
      break;
  }
}

void ble_set_app_chosen_cb(ble_app_chosen_cb_t cb){
  ble_app_chosen_cb = cb;
}

void ble_set_apps_requested_cb(ble_apps_requested_cb_t cb){
  ble_apps_requested_cb = cb;
}

void ble_set_input_received_cb(ble_input_received_cb_t cb){
  ble_input_received_cb = cb;
}

void ble_set_connected_cb(ble_connected_cb_t cb){
  ble_connected_cb = cb;
}

void ble_set_disconnected_cb(ble_disconnected_cb_t cb){
  ble_disconnected_cb = cb;
}
