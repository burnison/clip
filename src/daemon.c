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
