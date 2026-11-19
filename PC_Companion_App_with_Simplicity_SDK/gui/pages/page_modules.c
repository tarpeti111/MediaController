
#include "page_modules.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "../../model.h"
#include "../widgets/module_card.h"
#include "../gui_backend.h"
#include "../gui_modules.h"

#define MAX_MODULES 12
#define MODULES_PER_ROW 3

static void refresh_modules(gpointer data)
{
    GtkWidget *grid = GTK_WIDGET(data);

    GtkWidget *child;

    while ((child = gtk_widget_get_first_child(grid)) != NULL) {
        gtk_grid_remove(GTK_GRID(grid), child);
    }

    size_t count = gui_modules_get_count();

    for (size_t i = 0; i < count; i++) {
        const gui_module_info_t *module =
            gui_modules_get_at(i);

        if (module == NULL)
            continue;

        GtkWidget *card = module_card_create(
            module->id,
            module->name,
            module->icon,
            module->description,
            module->version
        );

        int column = i % MODULES_PER_ROW;
        int row = i / MODULES_PER_ROW;

        gtk_grid_attach(
            GTK_GRID(grid),
            card,
            column,
            row,
            1,
            1
        );
    }
}

GtkWidget *modules_create(void)
{
    GtkWidget *page =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_margin_start(page, 24);
    gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 24);

    GtkWidget *title =
        gtk_label_new("Installed Modules");

    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page), title);

    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_box_append(GTK_BOX(page), grid);

    gui_backend_set_modules_changed_callback(
        refresh_modules,
        grid
    );

    refresh_modules(grid);

    return page;
}
