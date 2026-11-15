#ifndef MODULE_CARD_H
#define MODULE_CARD_H

#include <gtk/gtk.h>

GtkWidget *module_card_create(
    const char *name,
    const char *description,
    const char *version
);

#endif