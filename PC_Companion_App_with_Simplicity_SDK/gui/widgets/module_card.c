#include "module_card.h"
#include "../gui_backend.h"

static void on_module_settings(GtkButton *button, gpointer data)
{
    const char *name = data;

    g_print("Settings for module: %s\n", name);
}

static void on_module_delete(GtkButton *button, gpointer data)
{
    const char *id = data;

    gui_backend_send_command(
        GUI_CMD_DELETE_MODULE,
        id
    );
}

GtkWidget *module_card_create(
    const char *id,
    const char *name,
    const char *icon,
    const char *description,
    const char *version)
{
    GtkWidget *card =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_add_css_class(card, "module-card");

    gtk_widget_set_size_request(card, 250, 160);
    
    GtkWidget *icon_widget = NULL;

    if (icon != NULL && icon[0] != '\0') {
        char *icon_path = g_build_filename(
            "modules",
            id,
            icon,
            NULL
        );

        if (g_file_test(icon_path, G_FILE_TEST_IS_REGULAR)) {
            icon_widget = gtk_image_new_from_file(icon_path);
        }

        g_free(icon_path);
    }

    if (icon_widget == NULL) {
        icon_widget = gtk_image_new_from_icon_name(
            "application-x-executable-symbolic"
        );
    }

    gtk_image_set_pixel_size(GTK_IMAGE(icon_widget), 48);

    GtkWidget *heading =
        gtk_label_new(name);

    gtk_widget_add_css_class(heading, "heading");
    gtk_widget_set_halign(heading, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(icon_widget, GTK_ALIGN_CENTER);

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


    g_signal_connect_data(
        manage_button,
        "clicked",
        G_CALLBACK(on_module_settings),
        g_strdup(name),
        (GClosureNotify)g_free,
        0
    );

    g_signal_connect_data(
        delete_button,
        "clicked",
        G_CALLBACK(on_module_delete),
        g_strdup(id),
        (GClosureNotify)g_free,
        0
    );

    gtk_box_append(GTK_BOX(button_box), manage_button);
    gtk_box_append(GTK_BOX(button_box), delete_button);

    gtk_box_append(GTK_BOX(card), icon_widget);
    gtk_box_append(GTK_BOX(card), heading);
    gtk_box_append(GTK_BOX(card), desc);
    gtk_box_append(GTK_BOX(card), version_label);
    gtk_box_append(GTK_BOX(card), button_box);

    return card;
}