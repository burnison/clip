#include "clipboard.h"
#include "daemon.h"
#include "gui.h"
#include "utils.h"

#include <gtk/gtk.h>

int main(int argc, char** argv)
{
    debug("Starting up.\n");

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

    return 0;
}
