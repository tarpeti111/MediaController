
#include "page_overview.h"

#include <stdio.h>

#include "../gui_modules.h"
#include "../gui_backend.h"

static GtkWidget *installed_modules_label = NULL;

static void refresh_module_count(gpointer data)
{
    char value[32];

    snprintf(
        value,
        sizeof(value),
        "%zu",
        gui_modules_get_count()
    );

    gtk_label_set_text(
        GTK_LABEL(installed_modules_label),
        value
    );
}

static GtkWidget *create_stat_card(
    const char *title,
    size_t value,
    GtkWidget **value_label)
{
    GtkWidget *card =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    gtk_widget_add_css_class(card, "stat-card");

    GtkWidget *heading = gtk_label_new(title);

    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%zu", value);

    GtkWidget *number = gtk_label_new(value_str);

    gtk_widget_add_css_class(number, "stat-value");

    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    gtk_widget_set_halign(number, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(card), heading);
    gtk_box_append(GTK_BOX(card), number);

    if (value_label != NULL)
        *value_label = number;

    return card;
}

GtkWidget *overview_create(void)
{
    GtkWidget *page =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    gtk_widget_set_margin_start(page, 24);
    gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 24);

    GtkWidget *title =
        gtk_label_new("Overview");

    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(page), title);

    GtkWidget *stats =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);

    gtk_box_append(
        GTK_BOX(stats),
        create_stat_card(
            "Installed modules",
            gui_modules_get_count(),
            &installed_modules_label
        )
    );

    gtk_box_append(GTK_BOX(page), stats);

    gui_backend_set_modules_changed_callback(
        refresh_module_count,
        NULL
    );

    GtkWidget *recent =
        gtk_label_new("Welcome to MediaController!");

    gtk_widget_set_halign(recent, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(page), recent);

    return page;
}
