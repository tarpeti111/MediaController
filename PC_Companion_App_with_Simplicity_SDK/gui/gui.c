#include <gtk/gtk.h>
#include "window/main_window.h"
#include "util/stdout_capture.h"

static void load_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_path(
        provider,
        "gui/style/theme.css"
    );

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}

static void on_activate(
    GtkApplication *app,
    gpointer user_data)
{
    load_css();

    GtkWidget *window = main_window_create(app);

    if (!stdout_capture_start()) {
        g_warning("Failed to start stdout capture");
    }

    gtk_window_present(GTK_WINDOW(window));
}

int gui_main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "com.mediacontroller.desktop",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(
        app,
        "activate",
        G_CALLBACK(on_activate),
        NULL
    );

    int status = g_application_run(
        G_APPLICATION(app), argc, argv
    );

    g_object_unref(app);

    return status;
}