#include "status_indicator.h"

GtkWidget *status_indicator_create(void)
{
    GtkWidget *label =
        gtk_label_new("● Disconnected");

    gtk_widget_add_css_class(label, "status-offline");

    return label;
}

void status_indicator_set_connected(
    GtkWidget *widget,
    gboolean connected)
{
    gtk_label_set_text(
        GTK_LABEL(widget),
        connected ? "● Connected" : "● Disconnected"
    );

    gtk_widget_remove_css_class(
        widget,
        connected ? "status-offline" : "status-online"
    );

    gtk_widget_add_css_class(
        widget,
        connected ? "status-online" : "status-offline"
    );
}