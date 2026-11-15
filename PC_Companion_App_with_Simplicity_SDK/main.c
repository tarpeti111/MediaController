/***************************************************************************//**
 * @file
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#include <glib.h>
#include <stdio.h>

#include "system.h"
#include "app_signal.h"
#include "app.h"
#include "gui/gui.h"

static gint run = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_atomic_int_set(&run, 0);
}

typedef struct {
    int argc;
    char **argv;
} thread_args_t;

static unsigned __stdcall silabs_main(void *arg)
{
  thread_args_t *args = (thread_args_t *)arg;

  printf("[THREAD] Silicon Labs thread started\n");

  app_signal(SIGINT, signal_handler);
  app_signal(SIGTERM, signal_handler);

  printf("[THREAD] Initializing system...\n");

  sl_system_init();

  printf("[THREAD] System initialized\n");

  app_init(args->argc, args->argv);

  printf("[THREAD] Application initialized\n");

  while (g_atomic_int_get(&run)) {
    sl_system_process_action();
    app_process_action();
  }

  printf("[THREAD] Deinitializing...\n");

  app_deinit();

  return 0;
}

int main(int argc, char *argv[])
{
  static thread_args_t args;

  args.argc = argc;
  args.argv = argv;

  // Start Silicon Labs worker thread.
  uintptr_t thread_id = _beginthreadex(
      NULL,
      0,
      silabs_main,
      &args,
      0,
      NULL
  );

  if (thread_id == 0) {
      return EXIT_FAILURE;
  }

  // GTK runs on the main thread.
  int status = gui_main(0, NULL);

  // Request Silicon Labs shutdown.
  g_atomic_int_set(&run, 0);

  // Wait for Silicon Labs to finish app_deinit().
  HANDLE thread = (HANDLE)thread_id;

  WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);

  return status;
}
