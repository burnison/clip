#include "clipboard.h"
#include "daemon.h"
#include "gui.h"
#include "utils.h"

#include <gtk/gtk.h>

int main(int argc, char** argv)
{
    debug("Starting up.\n");

    gtk_init(&argc, &argv);

    GtkClipboard* provider = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    Clipboard* clipboard = clip_clipboard_new(provider);

    clip_daemon_init(clipboard);
    clip_daemon_start();
    clip_gui_init();

    gtk_main();

    clip_gui_destroy();
    clip_clipboard_free(clipboard);

    return 0;
}
