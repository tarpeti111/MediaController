#include "page_settings.h"

GtkWidget *settings_create(void)
{
    GtkWidget *page =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_margin_start(page, 24);
    gtk_widget_set_margin_top(page, 24);

    GtkWidget *title =
        gtk_label_new("Settings");

    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    GtkWidget *debug =
        gtk_check_button_new_with_label(
            "Enable debug logging"
        );

    gtk_box_append(GTK_BOX(page), title);
    gtk_box_append(GTK_BOX(page), debug);

    return page;
}