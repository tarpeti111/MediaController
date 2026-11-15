#include "log_viewer.h"

static GtkTextBuffer *buffer = NULL;

GtkWidget *log_viewer_create(void)
{
    GtkWidget *scroll =
        gtk_scrolled_window_new();

    gtk_widget_set_size_request(scroll, -1, 180);

    GtkWidget *text =
        gtk_text_view_new();

    gtk_text_view_set_editable(
        GTK_TEXT_VIEW(text), FALSE
    );

    gtk_text_view_set_monospace(
        GTK_TEXT_VIEW(text), TRUE
    );

    gtk_text_view_set_cursor_visible(
        GTK_TEXT_VIEW(text), FALSE
    );

    buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scroll), text
    );

    return scroll;
}

void log_viewer_append(const char *message)
{
    if (!buffer)
        return;

    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_insert(
        buffer,
        &end,
        message,
        -1
    );

    gtk_text_buffer_insert(
        buffer,
        &end,
        "\n",
        1
    );
}