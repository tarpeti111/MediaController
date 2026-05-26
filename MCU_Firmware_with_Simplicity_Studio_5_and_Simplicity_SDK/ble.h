/*
 * ble.h
 *
 *  Created on: Mar 30, 2026
 *      Author: tarpe
 */

#ifndef BLE_H_
#define BLE_H_

#include "gatt_db.h"
#include "sl_status.h"

void ble_connect();
void ble_disconnect();

sl_status_t ble_write_app_choice();
sl_status_t ble_notify_input_states(uint8_t input_states[gattdb_input_states_len]);

#endif /* BLE_H_ */
