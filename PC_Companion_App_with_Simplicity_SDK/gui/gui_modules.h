#ifndef GUI_MODULES_H
#define GUI_MODULES_H

#include <stdbool.h>
#include <stddef.h>

#define GUI_MODULE_ID_LENGTH 32
#define GUI_MODULE_NAME_LENGTH 64
#define GUI_MODULE_ICON_LENGTH 128
#define GUI_MODULE_VERSION_LENGTH 16
#define GUI_MODULE_DESCRIPTION_LENGTH 256

typedef struct {
    char id[GUI_MODULE_ID_LENGTH];
    char name[GUI_MODULE_NAME_LENGTH];
    char icon[GUI_MODULE_ICON_LENGTH];
    char version[GUI_MODULE_VERSION_LENGTH];
    char description[GUI_MODULE_DESCRIPTION_LENGTH];
} gui_module_info_t;

bool gui_modules_init(const char *modules_directory);

const gui_module_info_t *gui_modules_find_by_name(const char *name);
const gui_module_info_t *gui_modules_find_by_id(const char *id);

size_t gui_modules_get_count(void);

const gui_module_info_t *gui_modules_get_at(size_t index);

#endif