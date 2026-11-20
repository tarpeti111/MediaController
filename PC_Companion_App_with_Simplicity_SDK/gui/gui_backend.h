#ifndef GUI_BACKEND_H
#define GUI_BACKEND_H

#include <gtk/gtk.h>

/*
 * Bridge from the Silicon Labs thread to the GUI thread.
 * Functions called from the backend only schedule work with g_idle_add(),
 * so GTK is only ever touched on the GUI thread.
 */

/* GUI thread: register the widget that shows the connection state. */
void gui_backend_bind_status(GtkWidget *widget);

/* Any thread: update the connection state shown in the GUI. */
void gui_backend_set_connected(gboolean connected);

#endif
