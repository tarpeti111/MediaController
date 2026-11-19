
#include "module_manager.h"

#include <archive.h>
#include <archive_entry.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <stdio.h>
#include <string.h>

#include "../cJSON.h"

#include "../gui/gui_modules.h"
#include "../gui/gui_backend.h"

#define MODULES_DIRECTORY "modules"
#define MANIFEST_NAME "module.json"
#define MAX_MANIFEST_SIZE 4096
#define MAX_ARCHIVE_ENTRIES 1024
#define MAX_UNPACKED_SIZE (64 * 1024 * 1024)

static bool set_error(char **error_message, const char *message)
{
    if (error_message != NULL)
        *error_message = g_strdup(message);

    return false;
}

static bool valid_module_id(const char *id)
{
    if (id == NULL || id[0] == '\0' || strlen(id) >= 32)
        return false;

    for (size_t i = 0; id[i] != '\0'; i++) {
        if (!g_ascii_isalnum(id[i]) &&
            id[i] != '_' &&
            id[i] != '-') {
            return false;
        }
    }

    return true;
}

/* Normalize a ZIP path and reject unsafe paths. */
static char *clean_archive_path(const char *path)
{
    if (path == NULL || path[0] == '\0' ||
        path[0] == '/' || strchr(path, '\\') != NULL ||
        strchr(path, ':') != NULL) {
        return NULL;
    }

    char *result = g_strdup(path);
    size_t length = strlen(result);

    while (length > 0 && result[length - 1] == '/')
        result[--length] = '\0';

    if (length == 0) {
        g_free(result);
        return NULL;
    }

    char **parts = g_strsplit(result, "/", -1);

    for (size_t i = 0; parts[i] != NULL; i++) {
        if (parts[i][0] == '\0' ||
            g_str_equal(parts[i], ".") ||
            g_str_equal(parts[i], "..")) {
            g_strfreev(parts);
            g_free(result);
            return NULL;
        }
    }

    g_strfreev(parts);
    return result;
}

static bool read_manifest(
    struct archive *archive,
    struct archive_entry *entry,
    char **module_id,
    char **error_message)
{
    la_int64_t size = archive_entry_size(entry);

    if (size <= 0 || size >= MAX_MANIFEST_SIZE){
        return set_error(error_message, "Invalid module manifest size");
    }

    char buffer[MAX_MANIFEST_SIZE];
    size_t total = 0;

    while (total < (size_t)size) {
        la_ssize_t count = archive_read_data(
            archive, buffer + total, (size_t)size - total);

        if (count <= 0){
            return set_error(error_message, "Could not read module manifest");
        }

        total += (size_t)count;
    }

    buffer[total] = '\0';

    cJSON *json = cJSON_Parse(buffer);

    if (!cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return set_error(error_message, "Invalid module manifest JSON");
    }

    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");

    if (!cJSON_IsString(id) || !cJSON_IsString(name) || !valid_module_id(id->valuestring) || name->valuestring == NULL || name->valuestring[0] == '\0') {
        cJSON_Delete(json);
        return set_error(error_message, "Missing or invalid module ID/name");
    }

    *module_id = g_strdup(id->valuestring);

    cJSON_Delete(json);
    return true;
}

/*
 * Validate the ZIP before extracting anything.
 * Expected layout:
 *
 * spotify/
 *     module.json
 *     icon.png
 *     assets/...
 */
static bool validate_archive(
    struct archive *archive,
    char **folder_name,
    char **module_id,
    char **error_message)
{
    struct archive_entry *entry;
    char *root = NULL;
    bool manifest_found = false;
    guint entry_count = 0;
    guint64 unpacked_size = 0;
    int result;

    while ((result = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        if (++entry_count > MAX_ARCHIVE_ENTRIES) {
            set_error(error_message, "ZIP contains too many entries");
            goto fail;
        }

        char *path = clean_archive_path(archive_entry_pathname(entry));

        if (path == NULL) {
            set_error(error_message, "Unsafe path in ZIP archive");
            goto fail;
        }

        char *slash = strchr(path, '/');
        char *current_root = slash != NULL
            ? g_strndup(path, slash - path)
            : g_strdup(path);

        if (root == NULL) {
            root = g_strdup(current_root);
        } else if (!g_str_equal(root, current_root)) {
            g_free(current_root);
            g_free(path);
            set_error(error_message, "ZIP must contain exactly one top-level folder");
            goto fail;
        }

        g_free(current_root);

        mode_t type = archive_entry_filetype(entry);

        if (type != AE_IFDIR && type != AE_IFREG) {
            g_free(path);
            set_error(error_message, "ZIP contains an unsupported file type");
            goto fail;
        }

        if (slash == NULL && type != AE_IFDIR) {
            g_free(path);
            set_error(error_message,
                "Files must be inside the top-level folder");
            goto fail;
        }

        if (type == AE_IFREG) {
            la_int64_t size = archive_entry_size(entry);

            if (size < 0 ||
                unpacked_size + (guint64)size > MAX_UNPACKED_SIZE) {
                g_free(path);
                set_error(error_message, "ZIP exceeds the maximum unpacked size");
                goto fail;
            }

            unpacked_size += (guint64)size;

            char *manifest_path = g_strdup_printf(
                "%s/%s", root, MANIFEST_NAME);

            g_print("ZIP entry: '%s' | Expected manifest: '%s'\n",
                    path, manifest_path);

            bool is_manifest = g_str_equal(path, manifest_path);
            g_free(manifest_path);

            if (is_manifest) {
                if (manifest_found) {
                    g_free(path);
                    set_error(error_message, "Multiple manifests found");
                    goto fail;
                }

                if (!read_manifest(
                        archive, entry, module_id, error_message)) {
                    g_free(path);
                    goto fail;
                }

                manifest_found = true;
            } else {
                archive_read_data_skip(archive);
            }
        } else {
            archive_read_data_skip(archive);
        }

        g_free(path);
    }

    if (result != ARCHIVE_EOF) {
        set_error(error_message, "Could not read ZIP archive");
        goto fail;
    }

    if (root == NULL || !manifest_found) {
        set_error(error_message,
            "ZIP must contain a folder with module.json");
        goto fail;
    }

    if (!valid_module_id(root) || !g_str_equal(root, *module_id)) {
        set_error(error_message,
            "ZIP folder name must match the module ID");
        goto fail;
    }

    *folder_name = root;
    return true;

fail:
    g_free(root);
    g_clear_pointer(module_id, g_free);
    return false;
}

/* Extract the contents of the enclosing folder into destination. */
static bool extract_archive(
    struct archive *archive,
    const char *folder_name,
    const char *destination,
    char **error_message)
{
    struct archive_entry *entry;
    char *prefix = g_strconcat(folder_name, "/", NULL);
    int result;
    bool success = false;

    while ((result = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        char *path = clean_archive_path(archive_entry_pathname(entry));

        if (path == NULL) {
            set_error(error_message, "Unsafe path in ZIP archive");
            goto cleanup;
        }

        /* Skip the enclosing folder itself. */
        if (g_str_equal(path, folder_name)) {
            g_free(path);
            archive_read_data_skip(archive);
            continue;
        }

        if (!g_str_has_prefix(path, prefix)) {
            g_free(path);
            set_error(error_message, "Invalid path during extraction");
            goto cleanup;
        }

        const char *relative_path = path + strlen(prefix);

        if (*relative_path == '\0') {
            g_free(path);
            set_error(error_message, "Invalid extraction path");
            goto cleanup;
        }

        char *output_path = g_build_filename(destination, relative_path, NULL);

        mode_t type = archive_entry_filetype(entry);

        if (type == AE_IFDIR) {
            if (g_mkdir_with_parents(output_path, 0755) != 0) {
                g_free(output_path);
                g_free(path);
                set_error(error_message, "Could not create directory");
                goto cleanup;
            }

            archive_read_data_skip(archive);
        } else if (type == AE_IFREG) {
            char *parent = g_path_get_dirname(output_path);

            if (g_mkdir_with_parents(parent, 0755) != 0) {
                g_free(parent);
                g_free(output_path);
                g_free(path);
                set_error(error_message, "Could not create directory");
                goto cleanup;
            }

            g_free(parent);

            FILE *file = g_fopen(output_path, "wb");

            if (file == NULL) {
                g_free(output_path);
                g_free(path);
                set_error(error_message, "Could not create extracted file");
                goto cleanup;
            }

            char buffer[8192];
            la_ssize_t count;
            bool write_ok = true;

            while ((count = archive_read_data(archive, buffer, sizeof(buffer))) > 0) {
                if (fwrite(buffer, 1, (size_t)count, file) != (size_t)count) {
                    write_ok = false;
                    break;
                }
            }

            if (count < 0){
                write_ok = false;
            }

            if (fclose(file) != 0){
                write_ok = false;
            }

            if (!write_ok) {
                g_free(output_path);
                g_free(path);
                set_error(error_message, "Could not write extracted file");
                goto cleanup;
            }
        } else {
            g_free(output_path);
            g_free(path);
            set_error(error_message, "ZIP contains an unsupported file type");
            goto cleanup;
        }

        g_free(output_path);
        g_free(path);
    }

    if (result != ARCHIVE_EOF) {
        set_error(error_message, "Error while extracting ZIP archive");
        goto cleanup;
    }

    success = true;

cleanup:
    g_free(prefix);
    return success;
}

static void remove_directory(const char *path)
{
    GDir *dir = g_dir_open(path, 0, NULL);

    if (dir == NULL) {
        g_remove(path);
        return;
    }

    const char *name;

    while ((name = g_dir_read_name(dir)) != NULL) {
        char *child = g_build_filename(path, name, NULL);

        if (g_file_test(child, G_FILE_TEST_IS_DIR))
            remove_directory(child);
        else
            g_remove(child);

        g_free(child);
    }

    g_dir_close(dir);
    g_rmdir(path);
}

bool module_manager_install(
    const char *zip_path,
    char **error_message)
{
    if (error_message != NULL){
        *error_message = NULL;
    }

    if (zip_path == NULL || zip_path[0] == '\0')
        return set_error(error_message, "Invalid ZIP path");

    struct archive *archive = archive_read_new();

    archive_read_support_format_zip(archive);
    archive_read_support_filter_all(archive);

    if (archive_read_open_filename(archive, zip_path, 10240) != ARCHIVE_OK) {
        archive_read_free(archive);
        return set_error(error_message, "Could not open ZIP archive");
    }

    char *folder_name = NULL;
    char *module_id = NULL;

    bool valid = validate_archive(
        archive, &folder_name, &module_id, error_message);

    archive_read_close(archive);
    archive_read_free(archive);

    if (!valid) {
        g_free(folder_name);
        g_free(module_id);
        return false;
    }

    if (g_mkdir_with_parents(MODULES_DIRECTORY, 0755) != 0) {
        g_free(folder_name);
        g_free(module_id);
        return set_error(error_message, "Could not create modules directory");
    }

    char *destination = g_build_filename(MODULES_DIRECTORY, module_id, NULL);

    if (g_file_test(destination, G_FILE_TEST_EXISTS)) {
        g_free(destination);
        g_free(folder_name);
        g_free(module_id);
        return set_error(error_message, "Module is already installed");
    }

    /* Extract into a temporary directory first. */
    char *template = g_build_filename(
        MODULES_DIRECTORY, ".install-XXXXXX", NULL);

    char *temporary_directory = g_mkdtemp(template);

    if (temporary_directory == NULL) {
        g_free(template);
        g_free(destination);
        g_free(folder_name);
        g_free(module_id);
        return set_error(error_message, "Could not create temporary installation directory");
    }

    archive = archive_read_new();
    archive_read_support_format_zip(archive);
    archive_read_support_filter_all(archive);

    bool success = false;

    if (archive_read_open_filename(archive, zip_path, 10240) != ARCHIVE_OK) {
        set_error(error_message, "Could not reopen ZIP archive");
    } else {
        success = extract_archive(archive, folder_name, temporary_directory, error_message);
    }

    archive_read_close(archive);
    archive_read_free(archive);

    if (success) {
        GFile *source = g_file_new_for_path(temporary_directory);
        GFile *target = g_file_new_for_path(destination);
        GError *error = NULL;

        success = g_file_move(
            source, target, G_FILE_COPY_NONE,
            NULL, NULL, NULL, &error);

        if (!success) {
            if (error_message != NULL){
                *error_message = g_strdup(error->message);
            }

            g_clear_error(&error);
        }

                g_object_unref(source);
        g_object_unref(target);
    }

    if (success) {
        gui_modules_init(MODULES_DIRECTORY);
        gui_backend_notify_modules_changed();
    }

    if (!success){
        remove_directory(temporary_directory);
    }

    g_free(template);
    g_free(destination);
    g_free(folder_name);
    g_free(module_id);

    return success;
}

bool module_manager_remove(
    const char *module_id,
    char **error_message)
{
    if (error_message != NULL)
        *error_message = NULL;

    if (module_id == NULL || module_id[0] == '\0') {
        if (error_message != NULL){
            *error_message = g_strdup("Invalid module ID");
        }
        return false;
    }

    for (size_t i = 0; module_id[i] != '\0'; i++) {
        if (!g_ascii_isalnum(module_id[i]) &&
            module_id[i] != '_' &&
            module_id[i] != '-') {
            if (error_message != NULL){
                *error_message = g_strdup("Invalid module ID");
            }
            return false;
        }
    }

    char *module_path = g_build_filename(
        MODULES_DIRECTORY,
        module_id,
        NULL
    );

    if (!g_file_test(module_path, G_FILE_TEST_IS_DIR)) {
        if (error_message != NULL){
            *error_message = g_strdup("Module directory not found");
        }

        g_free(module_path);
        return false;
    }

    remove_directory(module_path);
    g_free(module_path);

    return true;
}
