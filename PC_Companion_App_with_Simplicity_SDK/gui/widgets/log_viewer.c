#include "log_viewer.h"

static GtkTextBuffer *buffer = NULL;
static GtkWidget *scrolled_window = NULL;
static GtkWidget *text_view = NULL;

// Text printed before the panel exists (e.g. during startup) is kept here
// and shown once the panel is created. Only used on the GUI thread.
static GString *backlog = NULL;

#define MAX_LOG_CHARS (512 * 1024)   // keep the panel from growing forever

static void insert_text(const char *text);

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

    if (backlog != NULL) {
        insert_text(backlog->str);
        g_string_free(backlog, TRUE);
        backlog = NULL;
    }

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

static void insert_text(const char *text)
{
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
    gtk_text_buffer_insert(buffer, &end, text, -1);

    // Drop the oldest text once the log gets too long
    int chars = gtk_text_buffer_get_char_count(buffer);
    if (chars > MAX_LOG_CHARS) {
        GtkTextIter start, cut;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_iter_at_offset(buffer, &cut, chars - MAX_LOG_CHARS);
        gtk_text_buffer_delete(buffer, &start, &cut);
    }

    if (was_at_bottom) {
        g_idle_add(scroll_to_bottom, buffer);
    }
}

// Appends raw text: '\n' starts a new line, text without it continues the
// current line (like a terminal).
void log_viewer_append_text(const char *text)
{
    if (text == NULL || text[0] == '\0')
        return;

    if (!buffer) {
        if (backlog == NULL)
            backlog = g_string_new(NULL);
        g_string_append(backlog, text);
        return;
    }

    insert_text(text);
}

// Appends one line (a '\n' is added).
void log_viewer_append(const char *message)
{
    log_viewer_append_text(message);
    log_viewer_append_text("\n");
}
