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

#include "daemon.h"
#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>


struct daemon {
    ClipboardProvider* provider;
    Clipboard* clipboard;
};


/**
 * @param daemon the GTK callback user data.
 */
static gboolean clip_daemon_poll(Daemon* daemon)
{
    ClipboardEntry* clipboard_entry = clip_clipboard_get(daemon->clipboard);
    char* clipboard_contents = clipboard_entry == NULL ? NULL : clip_clipboard_entry_get_text(clipboard_entry);
    char* provider_contents = clip_provider_get_current(daemon->provider);

    if(g_strcmp0(clipboard_contents, provider_contents)){
        trace("Provider clipboard contents differ from active clipboard.\n");
        clip_clipboard_set(daemon->clipboard, provider_contents);
    }

    clip_provider_free_current(provider_contents);
    clip_clipboard_entry_free(clipboard_entry);

    return TRUE;
}



static void clip_daemon_restart(Daemon* daemon)
{
    debug("Daemon apparently dead. Restarting.\n");
    clip_daemon_start(daemon);
}

void clip_daemon_start(Daemon* daemon)
{
    trace("Creating daemon task.\n");
    g_timeout_add_full(G_PRIORITY_DEFAULT, DAEMON_REFRESH_INTERVAL,
            (GSourceFunc)clip_daemon_poll, daemon, (GDestroyNotify)clip_daemon_restart);
}



Daemon* clip_daemon_new(ClipboardProvider* provider, Clipboard* clipboard)
{
    trace("Initializing daemon.\n");
    Daemon* daemon = g_malloc(sizeof(Daemon));
    daemon->provider = provider;
    daemon->clipboard = clipboard;
    return daemon;
}

void clip_daemon_free(Daemon* daemon)
{
    daemon->clipboard = NULL;
    daemon->provider = NULL;
    g_free(daemon);
}
