#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H

#include <gtk/gtk.h>

GtkWidget *log_viewer_create(void);

void log_viewer_append(const char *message);

#endif