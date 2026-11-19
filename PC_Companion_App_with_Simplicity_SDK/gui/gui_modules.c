#include "gui_modules.h"

#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "../cJSON.h"

#define JSON_BUFFER_SIZE 4096
#define MAX_MODULES 12

static gui_module_info_t module_info[MAX_MODULES];
static size_t module_count = 0;


static bool valid_module_id(const char *id)
{
    if (id == NULL || id[0] == '\0')
        return false;

    for (size_t i = 0; id[i] != '\0'; i++) {
        if (!g_ascii_isalnum(id[i]) &&
            id[i] != '_' &&
            id[i] != '-') {
            return false;
        }
    }

    return strlen(id) < GUI_MODULE_ID_LENGTH;
}

static bool valid_module_icon_path(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return false;

    if (strlen(path) >= GUI_MODULE_ICON_LENGTH)
        return false;

    /* Icon paths must be relative. */
    if (path[0] == '/' || path[0] == '\\')
        return false;

    const char *component = path;

    for (const char *p = path; ; p++) {
        if (*p == '\\' || *p == ':')
            return false;

        if (*p == '/' || *p == '\0') {
            size_t length = p - component;

            if (length == 0)
                return false;

            if ((length == 1 && component[0] == '.') ||
                (length == 2 && component[0] == '.' &&
                 component[1] == '.')) {
                return false;
            }

            if (*p == '\0')
                break;

            component = p + 1;
        } else if (!g_ascii_isalnum(*p) &&
                   *p != '_' &&
                   *p != '-' &&
                   *p != '.') {
            return false;
        }
    }

    return true;
}

static bool module_exists(const char *id)
{
    return gui_modules_find_by_id(id) != NULL;
}


static bool load_module_metadata(
    const char *path,
    const char *directory_id)
{
    if (module_count >= MAX_MODULES) {
        g_warning("Module limit reached");
        return false;
    }

    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        g_warning("Could not open module metadata: %s", path);
        return false;
    }

    char buffer[JSON_BUFFER_SIZE];

    size_t length = fread(
        buffer,
        1,
        sizeof(buffer) - 1,
        file
    );

    bool read_error = ferror(file);
    int extra_char = fgetc(file);

    fclose(file);

    if (read_error || extra_char != EOF) {
        g_warning("Could not read metadata or file is too large: %s", path);
        return false;
    }

    buffer[length] = '\0';

    cJSON *root = cJSON_Parse(buffer);

    if (!cJSON_IsObject(root)) {
        g_warning("Invalid JSON metadata: %s", path);
        cJSON_Delete(root);
        return false;
    }

    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    cJSON *description = cJSON_GetObjectItemCaseSensitive(root, "description");
    cJSON *icon = cJSON_GetObjectItemCaseSensitive(root, "icon");

    if (!cJSON_IsString(id) ||
        !cJSON_IsString(name) ||
        id->valuestring == NULL ||
        name->valuestring == NULL ||
        !valid_module_id(id->valuestring) ||
        name->valuestring[0] == '\0') {

        g_warning("Missing or invalid ID/name in: %s", path);
        cJSON_Delete(root);
        return false;
    }

    if (g_strcmp0(id->valuestring, directory_id) != 0) {
        g_warning(
            "Module ID '%s' does not match directory '%s'",
            id->valuestring,
            directory_id
        );

        cJSON_Delete(root);
        return false;
    }

    if (module_exists(id->valuestring)) {
        g_warning("Duplicate module ID: %s", id->valuestring);
        cJSON_Delete(root);
        return false;
    }

    gui_module_info_t *info = &module_info[module_count];

    memset(info, 0, sizeof(*info));

    g_strlcpy(info->id, id->valuestring, sizeof(info->id));
    g_strlcpy(info->name, name->valuestring, sizeof(info->name));

    if (cJSON_IsString(version) && version->valuestring != NULL) {
        g_strlcpy(
            info->version,
            version->valuestring,
            sizeof(info->version)
        );
    }

    if (cJSON_IsString(description) && description->valuestring != NULL) {
        g_strlcpy(
            info->description,
            description->valuestring,
            sizeof(info->description)
        );
    }

    if (cJSON_IsString(icon) && icon->valuestring != NULL &&
        valid_module_icon_path(icon->valuestring)) {

        char *module_directory = g_path_get_dirname(path);
        char *icon_path = g_build_filename(
            module_directory,
            icon->valuestring,
            NULL
        );

        if (g_file_test(icon_path, G_FILE_TEST_IS_REGULAR)) {
            g_strlcpy(
                info->icon,
                icon->valuestring,
                sizeof(info->icon)
            );
        } else {
            g_warning(
                "Module icon not found: %s",
                icon_path
            );
        }

        g_free(icon_path);
        g_free(module_directory);

    } else if (icon != NULL) {
        g_warning("Invalid icon path in module metadata: %s", path);
    }

    g_print("Discovered module: %s (%s)\n", info->name, info->id);

    module_count++;

    cJSON_Delete(root);

    return true;
}


bool gui_modules_init(const char *modules_directory)
{
    module_count = 0;
    memset(module_info, 0, sizeof(module_info));

    if (modules_directory == NULL)
        return false;

    GDir *directory = g_dir_open(modules_directory, 0, NULL);

    if (directory == NULL) {
        g_warning("Could not open modules directory: %s",
                  modules_directory);
        return false;
    }

    bool all_loaded = true;
    const char *entry;

    while ((entry = g_dir_read_name(directory)) != NULL) {
        char *module_path = g_build_filename(
            modules_directory,
            entry,
            NULL
        );

        if (!g_file_test(module_path, G_FILE_TEST_IS_DIR)) {
            g_free(module_path);
            continue;
        }

        char *metadata_path = g_build_filename(
            module_path,
            "module.json",
            NULL
        );

        if (!g_file_test(metadata_path, G_FILE_TEST_IS_REGULAR)) {
            g_warning("Missing module.json in: %s", module_path);
            all_loaded = false;
        } else if (!load_module_metadata(metadata_path, entry)) {
            all_loaded = false;
        }

        g_free(metadata_path);
        g_free(module_path);
    }

    g_dir_close(directory);

    return all_loaded;
}


const gui_module_info_t *gui_modules_find_by_name(const char *name)
{
    if (name == NULL)
        return NULL;

    for (size_t i = 0; i < module_count; i++) {
        if (g_strcmp0(module_info[i].name, name) == 0)
            return &module_info[i];
    }

    return NULL;
}


const gui_module_info_t *gui_modules_find_by_id(const char *id)
{
    if (id == NULL)
        return NULL;

    for (size_t i = 0; i < module_count; i++) {
        if (g_strcmp0(module_info[i].id, id) == 0)
            return &module_info[i];
    }

    return NULL;
}


size_t gui_modules_get_count(void)
{
    return module_count;
}


const gui_module_info_t *gui_modules_get_at(size_t index)
{
    if (index >= module_count)
        return NULL;

    return &module_info[index];
}