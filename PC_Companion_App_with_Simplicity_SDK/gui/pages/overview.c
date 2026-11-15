#include "overview.h"

static GtkWidget *create_stat_card(
    const char *title,
    const char *value)
{
    GtkWidget *card =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    gtk_widget_add_css_class(card, "stat-card");

    GtkWidget *heading = gtk_label_new(title);
    GtkWidget *number = gtk_label_new(value);

    gtk_widget_add_css_class(number, "stat-value");

    gtk_widget_set_halign(heading, GTK_ALIGN_START);
    gtk_widget_set_halign(number, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(card), heading);
    gtk_box_append(GTK_BOX(card), number);

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
        create_stat_card("Installed modules", "3")
    );

    gtk_box_append(
        GTK_BOX(stats),
        create_stat_card("Connected devices", "1")
    );

    gtk_box_append(
        GTK_BOX(page),
        stats
    );

    GtkWidget *recent =
        gtk_label_new("Welcome to MediaController!");

    gtk_widget_set_halign(recent, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(page), recent);

    return page;
}