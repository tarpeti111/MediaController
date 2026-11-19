#ifndef GUI_BACKEND_H
#define GUI_BACKEND_H

#include <stdbool.h>
#include <stdint.h>
#include <gtk/gtk.h>

/* Commands sent from the GUI to the backend */
typedef enum {
    GUI_CMD_DELETE_MODULE,
    GUI_CMD_INSTALL_MODULE,
    GUI_CMD_LAUNCH_MODULE
} gui_command_type_t;

typedef struct {
    gui_command_type_t type;
    char name[64];
} gui_command_t;

/* Existing backend functions */
void gui_backend_init(void);

void gui_backend_send_command(
    gui_command_type_t type,
    const char *name
);

gboolean gui_backend_get_command(
    gui_command_t *command
);

void gui_backend_set_connected(gboolean connected);
void gui_backend_bind_status(GtkWidget *widget);

/* Modules changed notification */
typedef void (*gui_modules_changed_cb)(gpointer data);

void gui_backend_set_modules_changed_callback(
    gui_modules_changed_cb callback,
    gpointer data
);

void gui_backend_notify_modules_changed(void);

#endif