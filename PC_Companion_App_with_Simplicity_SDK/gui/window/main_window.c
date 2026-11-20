#include "main_window.h"

#include "../pages/page_overview.h"
#include "../pages/page_modules.h"
#include "../pages/page_settings.h"
#include "../widgets/log_viewer.h"
#include "../widgets/status_indicator.h"
#include "../gui_backend.h"

static void navigate(GtkButton *button, gpointer data)
{
    GtkStack *stack = GTK_STACK(data);

    const char *page =
        g_object_get_data(G_OBJECT(button), "page");

    gtk_stack_set_visible_child_name(stack, page);
}

static GtkWidget *create_sidebar(GtkStack *stack)
{
    GtkWidget *sidebar =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

    gtk_widget_set_vexpand(sidebar, TRUE);
    gtk_widget_add_css_class(sidebar, "sidebar");
    gtk_widget_set_size_request(sidebar, 190, -1);
    gtk_widget_set_margin_start(sidebar, 12);
    gtk_widget_set_margin_end(sidebar, 12);
    gtk_widget_set_margin_top(sidebar, 16);

    const char *names[] = {
        "Overview",
        "Modules",
        "Settings"
    };

    const char *pages[] = {
        "overview",
        "modules",
        "settings"
    };

    for (size_t i = 0; i < G_N_ELEMENTS(pages); i++) {
        GtkWidget *button =
            gtk_button_new_with_label(names[i]);

        g_object_set_data(
            G_OBJECT(button),
            "page",
            (gpointer)pages[i]
        );

        g_signal_connect(
            button,
            "clicked",
            G_CALLBACK(navigate),
            stack
        );

        gtk_box_append(GTK_BOX(sidebar), button);
    }

    return sidebar;
}

static void toggle_debug(GtkToggleButton *button, gpointer data)
{
    GtkRevealer *revealer = GTK_REVEALER(data);

    gtk_revealer_set_reveal_child(
        revealer,
        gtk_toggle_button_get_active(button)
    );
}

GtkWidget *main_window_create(GtkApplication *app)
{
    GtkWidget *window =
        gtk_application_window_new(app);

    gtk_window_set_title(
        GTK_WINDOW(window),
        "MediaController"
    );

    gtk_window_set_default_size(
        GTK_WINDOW(window),
        1150,
        750
    );

    // Header
    GtkWidget *header = gtk_header_bar_new();

    GtkWidget *debug_button =
        gtk_toggle_button_new_with_label("Debug");

    gtk_header_bar_pack_end(
        GTK_HEADER_BAR(header),
        debug_button
    );

    GtkWidget *status =
        status_indicator_create();

    gui_backend_bind_status(status);

    gtk_header_bar_pack_start(
        GTK_HEADER_BAR(header),
        status
    );

    gtk_window_set_titlebar(
        GTK_WINDOW(window),
        header
    );

    // Main layout
    GtkWidget *root =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_window_set_child(GTK_WINDOW(window), root);

    GtkWidget *content =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_widget_set_vexpand(content, TRUE);

    gtk_box_append(GTK_BOX(root), content);

    // Pages
    GtkWidget *stack = gtk_stack_new();

    gtk_stack_set_transition_type(
        GTK_STACK(stack),
        GTK_STACK_TRANSITION_TYPE_CROSSFADE
    );

    gtk_stack_add_named(
        GTK_STACK(stack),
        overview_create(),
        "overview"
    );

    gtk_stack_add_named(
        GTK_STACK(stack),
        modules_create(),
        "modules"
    );

    gtk_stack_add_named(
        GTK_STACK(stack),
        settings_create(),
        "settings"
    );

    gtk_widget_set_hexpand(stack, TRUE);

    gtk_box_append(
        GTK_BOX(content),
        create_sidebar(GTK_STACK(stack))
    );

    gtk_box_append(GTK_BOX(content), stack);

    // Debug panel
    GtkWidget *revealer = gtk_revealer_new();

    gtk_revealer_set_transition_type(
        GTK_REVEALER(revealer),
        GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP
    );

    gtk_revealer_set_reveal_child(
        GTK_REVEALER(revealer),
        FALSE
    );

    gtk_revealer_set_child(
        GTK_REVEALER(revealer),
        log_viewer_create()
    );

    gtk_box_append(GTK_BOX(root), revealer);

    g_signal_connect(
        debug_button,
        "toggled",
        G_CALLBACK(toggle_debug),
        revealer
    );

    gtk_stack_set_visible_child_name(
        GTK_STACK(stack),
        "overview"
    );

    return window;
}