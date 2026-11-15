#include "module_card.h"

static void on_module_settings(GtkButton *button, gpointer data)
{
    const char *name = data;

    g_print("Settings for module: %s\n", name);
}

static void on_module_delete(GtkButton *button, gpointer data)
{
    const char *name = data;

    g_print("Delete module: %s\n", name);
}

GtkWidget *module_card_create(
    const char *name,
    const char *description,
    const char *version)
{
    GtkWidget *card =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_add_css_class(card, "module-card");

    gtk_widget_set_size_request(card, 250, 160);

    GtkWidget *heading =
        gtk_label_new(name);

    gtk_widget_add_css_class(heading, "heading");
    gtk_widget_set_halign(heading, GTK_ALIGN_START);

    GtkWidget *desc =
        gtk_label_new(description);

    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_widget_set_halign(desc, GTK_ALIGN_START);

    GtkWidget *version_label =
        gtk_label_new(version);

    gtk_widget_add_css_class(version_label, "dim-label");

    GtkWidget *button_box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);

    GtkWidget *manage_button =
        gtk_button_new_with_label("Manage");

    GtkWidget *delete_button =
        gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(delete_button, "destructive-action");


    g_signal_connect(
        manage_button,
        "clicked",
        G_CALLBACK(on_module_settings),
        (gpointer)name
    );

    g_signal_connect(
        delete_button,
        "clicked",
        G_CALLBACK(on_module_delete),
        (gpointer)name
    );

    gtk_box_append(GTK_BOX(button_box), manage_button);
    gtk_box_append(GTK_BOX(button_box), delete_button);

    gtk_box_append(GTK_BOX(card), heading);
    gtk_box_append(GTK_BOX(card), desc);
    gtk_box_append(GTK_BOX(card), version_label);
    gtk_box_append(GTK_BOX(card), button_box);

    return card;
}