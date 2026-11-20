#include "page_modules.h"

#include "../../modules/modules.h"
#include "../widgets/module_card.h"

#define MODULES_PER_ROW 3

GtkWidget *modules_create(void)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

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
    gtk_box_append(GTK_BOX(page), grid);

    /* Modules are compiled in, so the list never changes at runtime. */
    for (size_t i = 0; i < modules_count(); i++) {
        gtk_grid_attach(
            GTK_GRID(grid),
            module_card_create(modules_get(i)),
            (int)(i % MODULES_PER_ROW),
            (int)(i / MODULES_PER_ROW),
            1,
            1
        );
    }

    return page;
}
