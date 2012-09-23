#include "config.h"
#include "utils.h"

#include "clipboard.h"

#include <stdio.h>
#include <gtk/gtk.h>


static int initialized;
static Clipboard* clipboard;



static void poll()
{
    GtkClipboard* provider = clip_get_provider(clipboard);
    gchar* contents = gtk_clipboard_wait_for_text(provider);

    if(contents == NULL){
        debug("Cannot attain clipboard data.");
        return;
    }

    const char* current = clip_get_text(clipboard);

    if(g_strcmp0(current, contents)){
        clip_set_text(clipboard, contents);
        debug("Setting clipboard to '%s'.\n", contents);
    }

    if(contents != NULL){
        g_free(contents);
    }
}

static void initialize(void)
{
    debug("Initializing.\n");

    GtkClipboard* provider = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    clipboard = clip_new_clipboard(provider);

    initialized = 1;
}

void start(void)
{
    debug("Starting up.\n");

    if(!initialized){
        initialize();
    }

    g_timeout_add_full(G_PRIORITY_DEFAULT, REFRESH, (GSourceFunc)poll, NULL, (GDestroyNotify)start);
}

int main(int argc, char** argv)
{
    gtk_init(&argc, &argv);

    initialized = 0;
    start();

    gtk_main();

    return 0;
}
