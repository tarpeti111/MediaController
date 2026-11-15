#include "modules.h"
#include "../widgets/module_card.h"

GtkWidget *modules_create(void)
{
    GtkWidget *page =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_margin_start(page, 24);
    gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 24);

    GtkWidget *title = gtk_label_new("Installed Modules");

    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(page), title);

    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    gtk_grid_attach(
        GTK_GRID(grid),
        module_card_create(
            "Spotify",
            "Music control integration",
            "1.0.0"
        ),
        0, 0, 1, 1
    );

    gtk_grid_attach(
        GTK_GRID(grid),
        module_card_create(
            "Desktop",
            "Desktop control integration",
            "1.0.0"
        ),
        1, 0, 1, 1
    );

    gtk_box_append(GTK_BOX(page), grid);

    return page;
}