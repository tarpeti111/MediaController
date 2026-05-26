/*
 * display.c
 *
 *  Created on: Mar 25, 2026
 *      Author: tarpe
 */



#include "display.h"
#include "string.h"
#include "stdio.h"
#include "sl_assert.h"
#include "sl_board_control.h"

#define MAX_VISIBLE_LINES 10
#define MAX_VISIBLE_APPS 3

static uint8_t scroll_offset = 0;

static GLIB_Context_t glibContext;

void display_init(){
  sl_status_t sc = sl_board_enable_display();
  EFM_ASSERT(sc == SL_STATUS_OK);

  sc = DMD_init(0);
  EFM_ASSERT(sc == DMD_OK);

  EFM_ASSERT(sc == GLIB_OK);
  GLIB_contextInit(&glibContext);
}

EMSTATUS display_clear(){
  return GLIB_clear(&glibContext);
}

EMSTATUS display_update(){
  return DMD_updateDisplay();
}

static void set_scroll_offset(uint8_t cursor, uint8_t max_items){
  if(cursor < scroll_offset){
        scroll_offset = cursor;
    }

    else if(cursor >= scroll_offset + max_items){
        scroll_offset = cursor - (max_items - 1);
    }
}

void display_draw_apps(app_t* apps, uint8_t app_count, uint8_t cursor_position){
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  uint8_t y_offset = 1;

  set_scroll_offset(cursor_position, 3);

  for (size_t i = scroll_offset; i < app_count && i < (scroll_offset + MAX_VISIBLE_APPS); i++) {
    app_t* app = &apps[i];

    if(app->id == 0xFF) return;

    if(i == cursor_position){
        GLIB_drawString(&glibContext, ">", 48, 16, y_offset+16, 0);
    }
    GLIB_drawString(&glibContext, app->name, strlen(app->name), (48+16)-(4*strlen(app->name)), y_offset+32+2, 0);
    GLIB_drawBitmap(&glibContext, 48, y_offset, 32, 32, app->bitmap);

    y_offset += 32 + 2 + 8 + 2;
  }
}

void display_no_address_found(){
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);

  char str[] = "NO ADDRESSES\nFOUND";
  GLIB_drawString(&glibContext, str, strlen(str), 0, 10, 0);
}

void display_no_apps_found(){
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);

  char str[] = "NO APPS\nFOUND";
  GLIB_drawString(&glibContext, str, strlen(str), 0, 10, 0);
}

void display_draw_addresses(bd_addr* addresses, uint8_t address_count, uint8_t cursor_position){
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);

  set_scroll_offset(cursor_position, MAX_VISIBLE_LINES/2);

  for (size_t i = scroll_offset; i < address_count && i < (scroll_offset + (MAX_VISIBLE_LINES / 2)); i++) {
      char str[24];
      bd_addr *a = &addresses[i];
      // Format address (reverse order is typical for BLE MAC display)
      snprintf(str, sizeof(str),
               "  %02X:%02X:%02X:\n  %02X:%02X:%02X",
               a->addr[5], a->addr[4], a->addr[3],
               a->addr[2], a->addr[1], a->addr[0]);
      if(i == cursor_position){
          str[0] = '>';
      }

      // Draw each address on a new line (8px font height)
      GLIB_drawString(&glibContext,
                      str,
                      strlen(str),
                      0,
                      2 + (i - scroll_offset) * 22,
                      0);
  }
}

void display_draw_address_names(char address_names[][20],
                               uint8_t address_count,
                               uint8_t cursor_position)
{
    glibContext.backgroundColor = White;
    glibContext.foregroundColor = Black;
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);

    uint32_t y_offset = 2;

    set_scroll_offset(cursor_position, MAX_VISIBLE_LINES/2);

    for (size_t i = scroll_offset; i < address_count && i < (scroll_offset + (MAX_VISIBLE_LINES / 2)); i++) {
        char str[26] = {'\0'};
        str[0] = ((int8_t)i == cursor_position) ? '>' : ' ';
        str[1] = ' ';

        char* src = address_names[i];

        size_t j = 0;
        for(; j < 14; j++){
            char c = src[j];
            str[j+2] = c;
            if(c == '\0') break;
        }
        if(j == 14 && str[j+1] != '\0'){
            str[j+2] = '\n';
            str[j+3] = ' ';
            str[j+4] = ' ';
            for(size_t k = j; j <= 20; j++){
                char c = src[j];
                if(c != ' ' || j >= 16){
                    str[k+5] = c;
                    k++;
                }
                if(c == '\0') break;
            }
        }
        GLIB_drawString(&glibContext, str, sizeof(str), 0, y_offset, 0);
        y_offset += (strchr(str, '\n') ? 20 : 20) + 2;
    }
}

void display_draw_connection_ui(){
  char* str_select = "SELECT";
  char* str_refresh = "REFRESH";
  int right_x = glibContext.pDisplayGeometry->xSize - (glibContext.font.fontWidth * strlen(str_refresh));
  uint8_t padding = 2;
  uint8_t y = glibContext.pDisplayGeometry->ySize - glibContext.font.fontHeight - padding;
  GLIB_drawString(&glibContext, str_select, strlen(str_select), padding, y, false);
  GLIB_drawString(&glibContext, str_refresh, strlen(str_refresh), right_x-padding, y, false);
}

#define UI_Y_OFFSET 114

const int32_t stop_button_left_points[] = {
    64+4, UI_Y_OFFSET+6,
    64+4, UI_Y_OFFSET-6,
    64+2, UI_Y_OFFSET+6,
    64+2, UI_Y_OFFSET-6
};

const int32_t stop_button_right_points[] = {
    64-4, UI_Y_OFFSET+6,
    64-4, UI_Y_OFFSET-6,
    64-2, UI_Y_OFFSET+6,
    64-2, UI_Y_OFFSET-6
};

static void ui_draw_stop_button(){
  GLIB_drawPolygonFilled(&glibContext, 4, stop_button_left_points);
  GLIB_drawPolygonFilled(&glibContext, 4, stop_button_right_points);
}


const int32_t play_button_points[] = {
    64+4, UI_Y_OFFSET+6,
    64+4, UI_Y_OFFSET-6,
    64+2, UI_Y_OFFSET+6,
    64+2, UI_Y_OFFSET-6
};

const int32_t skip_forward_button_points[] = {
    86-8, UI_Y_OFFSET+6,
    86-8, UI_Y_OFFSET-6,
    86, UI_Y_OFFSET-1,
    86, UI_Y_OFFSET-6,
    86+8, UI_Y_OFFSET,
    86, UI_Y_OFFSET+6,
    86, UI_Y_OFFSET+1
};

const int32_t skip_backward_button_points[] = {
    42+8, UI_Y_OFFSET+6,
    42+8, UI_Y_OFFSET-6,
    42, UI_Y_OFFSET-1,
    42, UI_Y_OFFSET-6,
    42-8, UI_Y_OFFSET,
    42, UI_Y_OFFSET+6,
    42, UI_Y_OFFSET+1
};

const GLIB_Rectangle_t rect = {64-96/2, 4, 64+96/2, 100};

void display_draw_app_ui(){
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_drawRect(&glibContext, &rect);
  GLIB_drawCircleFilled(&glibContext, 64, UI_Y_OFFSET, 10);
  GLIB_drawPolygonFilled(&glibContext, 7, skip_forward_button_points);
  GLIB_drawPolygonFilled(&glibContext, 7, skip_backward_button_points);
  glibContext.backgroundColor = Black;
  glibContext.foregroundColor = White;
  //GLIB_drawPolygonFilled(&glibContext, 3, play_button_points);
  ui_draw_stop_button();
}
