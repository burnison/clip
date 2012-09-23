#include "daemon.h"

#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>


static Clipboard* clipboard;


static gboolean clip_daemon_poll(gpointer data)
{
    GtkClipboard* provider = clip_clipboard_get_provider(clipboard);
    gchar* contents = gtk_clipboard_wait_for_text(provider);

    if(contents == NULL){
        debug("Cannot attain clipboard data.");
        return TRUE;
    }

    const char* current = clip_clipboard_get_text(clipboard);

    if(g_strcmp0(current, contents)){
        debug("Setting clipboard to '%s'.\n", contents);
        clip_clipboard_set_text(clipboard, contents);
    }

    if(contents != NULL){
        g_free(contents);
    }

    return TRUE;
}

static void clip_daemon_restart(void)
{
    debug("Daemon apparently dead. Restarting.\n");
    clip_daemon_start();
}

void clip_daemon_start(void)
{
    g_timeout_add_full(G_PRIORITY_DEFAULT, REFRESH, (GSourceFunc)clip_daemon_poll, NULL, (GDestroyNotify)clip_daemon_restart);
}

void clip_daemon_init(Clipboard* _clipboard)
{
    debug("Initializing daemon.\n");

    clipboard = _clipboard;
}
