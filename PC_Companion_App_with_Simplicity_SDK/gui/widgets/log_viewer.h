#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H

#include <gtk/gtk.h>

GtkWidget *log_viewer_create(void);

// Appends one line (a '\n' is added).
void log_viewer_append(const char *message);

// Appends raw text as printed (used by the stdout/stderr capture).
void log_viewer_append_text(const char *text);

#endif