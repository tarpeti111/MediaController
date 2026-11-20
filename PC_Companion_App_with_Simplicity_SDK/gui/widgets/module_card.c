#include "module_card.h"

static void on_module_settings(GtkButton *button, gpointer data)
{
    const module_descriptor_t *module = data;

    g_print("Settings for module: %s\n", module->name);
}

static GtkWidget *create_icon(const char *icon_path)
{
    GtkWidget *icon = NULL;

    if (icon_path != NULL &&
        g_file_test(icon_path, G_FILE_TEST_IS_REGULAR)) {
        icon = gtk_image_new_from_file(icon_path);
    }

    if (icon == NULL)
        icon = gtk_image_new_from_icon_name("application-x-executable-symbolic");

    gtk_image_set_pixel_size(GTK_IMAGE(icon), 48);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);

    return icon;
}

GtkWidget *module_card_create(const module_descriptor_t *module)
{
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_add_css_class(card, "module-card");
    gtk_widget_set_size_request(card, 250, 160);

    GtkWidget *heading = gtk_label_new(module->name);

    gtk_widget_add_css_class(heading, "heading");
    gtk_widget_set_halign(heading, GTK_ALIGN_CENTER);

    GtkWidget *desc = gtk_label_new(module->description);

    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_widget_set_halign(desc, GTK_ALIGN_START);

    GtkWidget *version = gtk_label_new(module->version);

    gtk_widget_add_css_class(version, "dim-label");

    GtkWidget *manage_button = gtk_button_new_with_label("Manage");

    gtk_widget_set_halign(manage_button, GTK_ALIGN_END);

    /* Descriptors are static const, so passing the pointer is safe. */
    g_signal_connect(
        manage_button,
        "clicked",
        G_CALLBACK(on_module_settings),
        (gpointer)module
    );

    gtk_box_append(GTK_BOX(card), create_icon(module->icon));
    gtk_box_append(GTK_BOX(card), heading);
    gtk_box_append(GTK_BOX(card), desc);
    gtk_box_append(GTK_BOX(card), version);
    gtk_box_append(GTK_BOX(card), manage_button);

    return card;
}
