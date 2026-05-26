/*
 * display.h
 *
 *  Created on: Mar 25, 2026
 *      Author: tarpe
 */

#ifndef BLE_H_
#define BLE_H_

#include "glib.h"
#include "dmd.h"
#include "sl_bgapi.h"
#include "model.h"

void display_init();

EMSTATUS display_clear();
EMSTATUS display_update();

void display_draw_addresses(bd_addr* adresses, uint8_t address_count, uint8_t cursor_position);
void display_draw_address_names(char address_names[][20],
                               uint8_t address_count,
                               uint8_t cursor_position);
void display_draw_connection_ui();
void display_no_address_found();

void display_draw_apps(app_t* apps, uint8_t app_count, uint8_t cursor_position);
void display_no_apps_found();
void display_draw_app_ui();


#endif /* BLE_H_ */
