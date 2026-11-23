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
#define fd_write(fd, buf, size) _write(fd, buf, (unsigned int)(size))
#define fd_close(fd) _close(fd)
#define fd_dup(fd) _dup(fd)
#define fd_dup2(oldfd, newfd) _dup2(oldfd, newfd)
#define create_pipe(fd) _pipe(fd, 4096, _O_BINARY)
#else
#include <unistd.h>
#define pipe_read(fd, buf, size) read(fd, buf, size)
#define fd_write(fd, buf, size) write(fd, buf, size)
#define fd_close(fd) close(fd)
#define fd_dup(fd) dup(fd)
#define fd_dup2(oldfd, newfd) dup2(oldfd, newfd)
#define create_pipe(fd) pipe(fd)
#endif

/*
 * stdout and stderr are redirected into a pipe. A reader thread copies
 * everything it reads
 *   - to the original console (so the terminal still shows the output), and
 *   - to the debug panel, as raw text: text without '\n' shows up right away
 *     and the next print continues the same line, like in a terminal.
 */

static int read_fd = -1;
static int console_fd = -1;   // original stdout, -1 if there is none

static gboolean append_log_text(gpointer data)
{
    char *text = data;

    log_viewer_append_text(text);

    g_free(text);

    return G_SOURCE_REMOVE;
}

static gpointer stdout_reader(gpointer data)
{
    (void)data;
    char buffer[512];

    while (1) {
        int count = pipe_read(read_fd, buffer, sizeof(buffer));

        if (count <= 0)
            break;

        // Terminal: unchanged bytes
        if (console_fd >= 0) {
            fd_write(console_fd, buffer, count);
        }

        // Debug panel: same text without '\r' (Windows line endings)
        GString *text = g_string_sized_new(count);
        for (int i = 0; i < count; i++) {
            if (buffer[i] != '\r') {
                g_string_append_c(text, buffer[i]);
            }
        }

        if (text->len > 0) {
            g_idle_add(append_log_text, g_string_free(text, FALSE));
        } else {
            g_string_free(text, TRUE);
        }
    }

    return NULL;
}

bool stdout_capture_start(void)
{
    int pipe_fd[2];

    fflush(stdout);
    fflush(stderr);

    if (create_pipe(pipe_fd) != 0)
        return false;

    read_fd = pipe_fd[0];
    console_fd = fd_dup(1);   // keep the console for the tee

    if (fd_dup2(pipe_fd[1], 1) < 0) {
        fd_close(pipe_fd[0]);
        fd_close(pipe_fd[1]);
        if (console_fd >= 0) {
            fd_close(console_fd);
            console_fd = -1;
        }
        read_fd = -1;
        return false;
    }

    // stderr too (g_warning, fprintf(stderr, ...)); failure is not fatal
    fd_dup2(pipe_fd[1], 2);

    fd_close(pipe_fd[1]);

    // Unbuffered, so output appears immediately and isn't lost on a crash
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    GThread *thread = g_thread_new(
        "stdout-reader",
        stdout_reader,
        NULL
    );

    g_thread_unref(thread);

    return true;
}
