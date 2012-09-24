#include "provider.h"

#include <gtk/gtk.h>

struct provider {
    GtkClipboard* clipboard;
};

ClipboardProvider* clip_provider_new()
{
    ClipboardProvider* provider = g_malloc(sizeof(ClipboardProvider));
    provider->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

    return provider;
}

void clip_provider_free(ClipboardProvider* provider)
{
    g_free(provider);
}

char* clip_provider_get_current(ClipboardProvider* provider)
{
    return gtk_clipboard_wait_for_text(provider->clipboard);
}
void clip_provider_free_current(char* current)
{
    g_free(current);
}


void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
    gtk_clipboard_set_text(provider->clipboard, text, -1);
}
