#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <gtk/gtk.h>

GtkWidget *status_indicator_create(void);

void status_indicator_set_connected(
    GtkWidget *widget,
    gboolean connected
);

#endif