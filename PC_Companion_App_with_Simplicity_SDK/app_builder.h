#include "stdint.h"

#define APP_ID_DEFAULT 0xFF

typedef void (*app_on_input_t)(uint16_t input);
typedef void (*app_on_app_launch_t)(void);
typedef void (*app_on_app_process_action_t)(void);

typedef struct __attribute__((packed)){
  char name[16];
  uint8_t id;
  uint8_t bitmap[128];
}app_t;

typedef struct {
  app_t app;
  app_on_app_launch_t on_app_launch;
  app_on_input_t on_input;
  app_on_app_process_action_t on_app_process_action;
  
}extended_app_t;

size_t app_builder_bitstringToBytes(const char *input, uint8_t *output);