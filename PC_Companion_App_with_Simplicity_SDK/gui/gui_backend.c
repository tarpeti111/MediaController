#include "gui_backend.h"
#include "widgets/status_indicator.h"

static GtkWidget *status_widget = NULL;

static gboolean update_status(gpointer data)
{
    gboolean connected = GPOINTER_TO_INT(data);

    if (status_widget != NULL)
        status_indicator_set_connected(status_widget, connected);

    return G_SOURCE_REMOVE;
}

void gui_backend_bind_status(GtkWidget *widget)
{
    status_widget = widget;
}

void gui_backend_set_connected(gboolean connected)
{
    g_idle_add(update_status, GINT_TO_POINTER(connected));
}
