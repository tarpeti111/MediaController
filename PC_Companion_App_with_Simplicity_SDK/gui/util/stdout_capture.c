#include "stdout_capture.h"
#include "../widgets/log_viewer.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define pipe_read(fd, buf, size) _read(fd, buf, (unsigned int)(size))
#define pipe_close(fd) _close(fd)
#define stdout_dup() _dup(_fileno(stdout))
#define stdout_dup2(oldfd, newfd) _dup2(oldfd, newfd)
#define create_pipe(fd) _pipe(fd, 4096, _O_BINARY)
#else
#include <unistd.h>
#define pipe_read(fd, buf, size) read(fd, buf, size)
#define pipe_close(fd) close(fd)
#define stdout_dup() dup(STDOUT_FILENO)
#define stdout_dup2(oldfd, newfd) dup2(oldfd, newfd)
#define create_pipe(fd) pipe(fd)
#endif

static int read_fd = -1;

static gboolean append_log_line(gpointer data)
{
    char *line = data;

    log_viewer_append(line);

    g_free(line);

    return G_SOURCE_REMOVE;
}

static gpointer stdout_reader(gpointer data)
{
    char buffer[512];
    GString *pending = g_string_new(NULL);

    while (1) {
        int count = pipe_read(read_fd, buffer, sizeof(buffer));

        if (count <= 0)
            break;

        for (int i = 0; i < count; i++) {
            if (buffer[i] == '\n') {
                if (pending->len > 0 &&
                    pending->str[pending->len - 1] == '\r') {
                    g_string_truncate(pending, pending->len - 1);
                }

                g_idle_add(
                    append_log_line,
                    g_string_free(pending, FALSE)
                );

                pending = g_string_new(NULL);
            } else {
                g_string_append_c(pending, buffer[i]);
            }
        }
    }

    if (pending->len > 0) {
        g_idle_add(
            append_log_line,
            g_string_free(pending, FALSE)
        );
    } else {
        g_string_free(pending, TRUE);
    }

    return NULL;
}

bool stdout_capture_start(void)
{
    int pipe_fd[2];

    fflush(stdout);

    if (create_pipe(pipe_fd) != 0)
        return false;

    read_fd = pipe_fd[0];

    if (stdout_dup2(pipe_fd[1], 1) < 0) {
        pipe_close(pipe_fd[0]);
        pipe_close(pipe_fd[1]);
        read_fd = -1;
        return false;
    }

    pipe_close(pipe_fd[1]);

    setvbuf(stdout, NULL, _IONBF, 0);

    GThread *thread = g_thread_new(
        "stdout-reader",
        stdout_reader,
        NULL
    );

    g_thread_unref(thread);

    return true;
}