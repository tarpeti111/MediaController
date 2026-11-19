#include "page_installer.h"

#include "../../modules/module_manager.h"

static void process_module_file(GFile *file)
{
    char *path = g_file_get_path(file);

    if (path == NULL) {
        g_warning("Could not get module file path");
        return;
    }

    g_print("Selected module: %s\n", path);

    char *error = NULL;

    if (module_manager_install(path, &error)) {
        g_print("Module installed successfully\n");
    } else {
        g_warning("Module installation failed: %s", error);
    }

    g_free(error);

    g_free(path);
}

static void on_file_selected(
    GtkNativeDialog *dialog,
    int response,
    gpointer user_data)
{
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *file = gtk_file_chooser_get_file(
            GTK_FILE_CHOOSER(dialog)
        );

        if (file != NULL) {
            process_module_file(file);
            g_object_unref(file);
        }
    }

    g_object_unref(dialog);
}

static void open_file_chooser(GtkWidget *widget, gpointer data)
{
    GtkWindow *parent = GTK_WINDOW(data);

    GtkFileChooserNative *chooser =
        gtk_file_chooser_native_new(
            "Select a module",
            parent,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Install",
            "Cancel"
        );

    GtkFileFilter *filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, "Module packages");

    // Change these patterns to match your eventual package format.
    gtk_file_filter_add_pattern(filter, "*.zip");
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_filter_add_pattern(filter, "*.module");

    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(chooser),
        filter
    );

    g_signal_connect(
        chooser,
        "response",
        G_CALLBACK(on_file_selected),
        NULL
    );

    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

static gboolean on_drop(
    GtkDropTarget *target,
    const GValue *value,
    double x,
    double y,
    gpointer data)
{
    GFile *file = g_value_get_object(value);

    if (file != NULL) {
        process_module_file(file);
        return TRUE;
    }

    return FALSE;
}

GtkWidget *installer_create(void)
{
    GtkWidget *page =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_margin_start(page, 24);
    gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 24);

    GtkWidget *title =
        gtk_label_new("Install Modules");

    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(page), title);

    GtkWidget *drop_area =
    gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_widget_add_css_class(drop_area, "drop-area");

    gtk_widget_set_size_request(drop_area, 500, 220);
    gtk_widget_set_halign(drop_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(drop_area, GTK_ALIGN_START);

    // Make the entire area clickable.
    GtkWidget *click_button =
        gtk_button_new();

    gtk_widget_add_css_class(click_button, "drop-button");
    gtk_widget_set_hexpand(click_button, TRUE);
    gtk_widget_set_vexpand(click_button, FALSE);    

    GtkWidget *content =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content, GTK_ALIGN_CENTER);

    GtkWidget *icon =
        gtk_image_new_from_icon_name(
            "folder-new"
        );

        gtk_image_set_pixel_size(GTK_IMAGE(icon), 48);
        
    GtkWidget *label =
        gtk_label_new("Drop a module here or click to browse");
    
    GtkWidget *description =
        gtk_label_new("Select a module package from your computer");
    
    gtk_widget_add_css_class(description, "dim-label");
    
    gtk_box_append(GTK_BOX(content), icon);
    gtk_box_append(GTK_BOX(content), label);
    gtk_box_append(GTK_BOX(content), description);
    
    gtk_button_set_child(GTK_BUTTON(click_button), content);
    
    gtk_box_append(GTK_BOX(drop_area), click_button);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(description, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(drop_area, 16);
        
    g_signal_connect(
        click_button,
        "clicked",
        G_CALLBACK(open_file_chooser),
        gtk_widget_get_root(page)
    );

    // Drag and drop support.
    GtkDropTarget *target = gtk_drop_target_new(
        G_TYPE_FILE,
        GDK_ACTION_COPY
    );

    g_signal_connect(
        target,
        "drop",
        G_CALLBACK(on_drop),
        NULL
    );

    gtk_widget_add_controller(
        drop_area,
        GTK_EVENT_CONTROLLER(target)
    );

    gtk_box_append(GTK_BOX(page), drop_area);

    return page;
}