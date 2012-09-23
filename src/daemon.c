#include "daemon.h"

#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>


struct subscriber {
    clipboard_changed_callback callback;
};

static GSList subscribers;


static ClipboardProvider* provider;
static Clipboard* clipboard;

static gboolean clip_daemon_poll(gpointer data)
{
    char* contents = clip_provider_get_current(provider);
    char* current = clip_clipboard_get_head(clipboard);

    if(g_strcmp0(current, contents)){
        debug("Setting clipboard to '%s'.\n", contents);
        clip_clipboard_set_head(clipboard, contents);
    }

    clip_provider_free_current(contents);

    return TRUE;
}

static void clip_daemon_restart()
{
    debug("Daemon apparently dead. Restarting.\n");
    clip_daemon_start();
}

void clip_daemon_start(void)
{
    g_timeout_add_full(G_PRIORITY_DEFAULT, REFRESH,
            (GSourceFunc)clip_daemon_poll, NULL, (GDestroyNotify)clip_daemon_restart);
}

void clip_daemon_init(ClipboardProvider* _provider, Clipboard* _clipboard)
{
    debug("Initializing daemon.\n");

    provider = _provider;
    clipboard = _clipboard;
//    subscribers = g_slist_alloc();
}

void clip_daemon_destroy()
{
    clipboard = NULL;
    provider = NULL;
}
