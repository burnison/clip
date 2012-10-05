/**
 * Copyright (C) 2012 Richard Burnison
 *
 * This file is part of Clip.
 *
 * Clip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Clip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Clip.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "clipboard.h"
#include "daemon.h"
#include "gui.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <stdlib.h>


static gboolean clip_main_init(void)
{
    const char* directory = clip_config_get_home_dir();
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


int main(int argc, char** argv)
{
    debug("Starting up.\n");
    if(!clip_main_init()){
        error("Cannot initialize application. Terminating.\n");
        exit(1);
    }

    gtk_init(&argc, &argv);

    ClipboardProvider* provider = clip_provider_new();
    Clipboard* clipboard = clip_clipboard_new(provider);

    Daemon* daemon = clip_daemon_new(provider, clipboard);
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
