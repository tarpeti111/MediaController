#include "gui_backend.h"
#include "widgets/status_indicator.h"

static GAsyncQueue *command_queue = NULL;
static GtkWidget *status_widget = NULL;

/* Module notification */
static GMutex modules_callback_mutex;

#define MAX_MODULE_CALLBACKS 8

typedef struct {
    gui_modules_changed_cb callback;
    gpointer data;
} module_callback_entry_t;

static module_callback_entry_t module_callbacks[MAX_MODULE_CALLBACKS];
static size_t module_callback_count = 0;

void gui_backend_init(void)
{
    command_queue = g_async_queue_new();
}

void gui_backend_send_command(gui_command_type_t type,
                              const char *name)
{
    gui_command_t *cmd = g_new0(gui_command_t, 1);

    cmd->type = type;

    if (name != NULL) {
        g_strlcpy(cmd->name, name, sizeof(cmd->name));
    }

    g_async_queue_push(command_queue, cmd);
}

gboolean gui_backend_get_command(gui_command_t *command)
{
    gui_command_t *cmd =
        g_async_queue_try_pop(command_queue);

    if (cmd == NULL)
        return FALSE;

    *command = *cmd;
    g_free(cmd);

    return TRUE;
}

/* Status indicator */

static gboolean update_status(gpointer data)
{
    gboolean connected = GPOINTER_TO_INT(data);

    if (status_widget != NULL) {
        status_indicator_set_connected(
            status_widget,
            connected
        );
    }

    return G_SOURCE_REMOVE;
}

void gui_backend_bind_status(GtkWidget *widget)
{
    status_widget = widget;
}

void gui_backend_set_connected(gboolean connected)
{
    g_idle_add(
        update_status,
        GINT_TO_POINTER(connected)
    );
}

/* Module notifications */

static gboolean dispatch_modules_changed(gpointer data)
{
    g_mutex_lock(&modules_callback_mutex);

    for (size_t i = 0; i < module_callback_count; i++) {
        if (module_callbacks[i].callback != NULL) {
            module_callbacks[i].callback(
                module_callbacks[i].data
            );
        }
    }

    g_mutex_unlock(&modules_callback_mutex);

    return G_SOURCE_REMOVE;
}

void gui_backend_set_modules_changed_callback(
    gui_modules_changed_cb callback,
    gpointer data)
{
    if (callback == NULL)
        return;

    g_mutex_lock(&modules_callback_mutex);

    if (module_callback_count < MAX_MODULE_CALLBACKS) {
        module_callbacks[module_callback_count].callback = callback;
        module_callbacks[module_callback_count].data = data;
        module_callback_count++;
    }

    g_mutex_unlock(&modules_callback_mutex);
}

void gui_backend_notify_modules_changed(void)
{
    g_idle_add(
        dispatch_modules_changed,
        NULL
    );
}
