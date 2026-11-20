#ifndef BLE_H
#define BLE_H

#include "sl_status.h"
#include "stdint.h"

typedef void (*ble_apps_requested_cb_t)(void);
typedef void (*ble_input_received_cb_t)(uint16_t input);
typedef void (*ble_connected_cb_t)(void);
typedef void (*ble_disconnected_cb_t)(void);
typedef void (*ble_app_chosen_cb_t)(uint8_t app_id);
typedef void (*ble_playlist_request_cb_t)(uint16_t request);

void ble_set_app_chosen_cb(ble_app_chosen_cb_t cb);
void ble_set_apps_requested_cb(ble_apps_requested_cb_t cb);
void ble_set_input_received_cb(ble_input_received_cb_t cb);
void ble_set_connected_cb(ble_connected_cb_t cb);
void ble_set_disconnected_cb(ble_disconnected_cb_t cb);
void ble_set_playlist_request_cb(ble_playlist_request_cb_t cb);

sl_status_t ble_send_app_info_data(const uint8_t* data);
sl_status_t ble_send_playlist_data(const uint8_t* data, uint8_t len);

#endif