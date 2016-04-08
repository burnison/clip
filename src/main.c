/*
 * Copyright (c) 2016 Richard Burnison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "clipboard.h"
#include "daemon.h"
#include "gui.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <stdlib.h>


static gboolean clip_main_init(void)
{
    const char *directory = clip_config_get_home_dir();
    if(g_file_test(directory, G_FILE_TEST_EXISTS)){
        if(!g_file_test(directory, G_FILE_TEST_IS_DIR)){
            warn("Application directory, %s exists, but is a normal file.", directory);
            return FALSE;
        }
    } else {
        return !g_mkdir(directory, 0750);
    }
    return TRUE;
}


int main(int argc, char **argv)
{
    debug("Starting up.\n");
    if(!clip_main_init()){
        error("Cannot initialize application. Terminating.\n");
        exit(1);
    }

    gtk_init(&argc, &argv);

    ClipboardProvider *provider = clip_provider_new();
    Clipboard *clipboard = clip_clipboard_new(provider);

    Daemon *daemon = clip_daemon_new(clipboard);
    clip_daemon_start(daemon);

    clip_gui_init(clipboard);

    gtk_main();

    clip_gui_destroy();
    clip_daemon_free(daemon);
    clip_clipboard_free(clipboard);
    clip_provider_free(provider);

    clip_config_destroy();

    return 0;
}
