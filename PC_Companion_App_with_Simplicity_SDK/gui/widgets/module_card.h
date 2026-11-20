#ifndef MODULE_CARD_H
#define MODULE_CARD_H

#include <gtk/gtk.h>

#include "../../modules/module_descriptor.h"

GtkWidget *module_card_create(const module_descriptor_t *module);

#endif
