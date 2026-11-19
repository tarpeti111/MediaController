#include "log_viewer.h"

static GtkTextBuffer *buffer = NULL;
static GtkWidget *scrolled_window = NULL;
static GtkWidget *text_view = NULL;

GtkWidget *log_viewer_create(void)
{

    scrolled_window = gtk_scrolled_window_new();

    gtk_widget_set_size_request(scrolled_window, -1, 180);

    text_view = gtk_text_view_new();

    gtk_text_view_set_editable(
        GTK_TEXT_VIEW(text_view), FALSE
    );

    gtk_text_view_set_monospace(
        GTK_TEXT_VIEW(text_view), TRUE
    );

    gtk_text_view_set_cursor_visible(
        GTK_TEXT_VIEW(text_view), FALSE
    );

    buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled_window), text_view
    );

    return scrolled_window;
}

static gboolean scroll_to_bottom(gpointer data)
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_view_scroll_to_iter(
        GTK_TEXT_VIEW(text_view),
        &end,
        0.0,
        FALSE,
        0.0,
        1.0
    );

    return G_SOURCE_REMOVE;
}

void log_viewer_append(const char *message)
{
    if (!buffer)
        return;

    GtkAdjustment *adjustment =
        gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(scrolled_window)
        );

    double value = gtk_adjustment_get_value(adjustment);
    double upper = gtk_adjustment_get_upper(adjustment);
    double page_size = gtk_adjustment_get_page_size(adjustment);

    gboolean was_at_bottom =
        value + page_size >= upper - 2.0;

    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", 1);

    if (was_at_bottom) {
        g_idle_add(scroll_to_bottom, buffer);
    }
}