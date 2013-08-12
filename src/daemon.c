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
    Clipboard *clipboard;
};


static gboolean clip_daemon_poll(Daemon *daemon)
{
    if(clip_clipboard_is_enabled(daemon->clipboard)){
        if(!clip_clipboard_is_synced_with_provider(daemon->clipboard)){
            trace("Provider clipboard contents differ from active clipboard.\n");
            clip_clipboard_sync_with_provider(daemon->clipboard);
        }
    }
    return TRUE;
}



static void clip_daemon_restart(Daemon *daemon)
{
    debug("Daemon apparently dead. Restarting.\n");
    clip_daemon_start(daemon);
}

void clip_daemon_start(Daemon *daemon)
{
    trace("Creating daemon task.\n");
    g_timeout_add_full(G_PRIORITY_DEFAULT, DAEMON_REFRESH_INTERVAL,
            (GSourceFunc)clip_daemon_poll, daemon, (GDestroyNotify)clip_daemon_restart);
}



Daemon* clip_daemon_new(Clipboard *clipboard)
{
    trace("Initializing daemon.\n");
    Daemon *daemon = g_malloc(sizeof(Daemon));
    daemon->clipboard = clipboard;
    return daemon;
}

void clip_daemon_free(Daemon *daemon)
{
    daemon->clipboard = NULL;
    g_free(daemon);
}
