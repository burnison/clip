#include "clipboard.h"

#include "utils.h"

#include <gtk/gtk.h>


struct clipboard {
    GtkClipboard* provider;
    char* text;
};


Clipboard* clip_clipboard_new(void* provider)
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->provider = provider;
    clipboard->text = g_malloc(0);

    return clipboard;
}

void clip_clipboard_free(Clipboard* clipboard)
{
    if(clipboard == NULL){
        return;
    }

    // Managed by GTK.
    clipboard->provider = NULL;

    g_free(clipboard->text);
    g_free(clipboard);
}

void* clip_clipboard_get_provider(Clipboard* clipboard)
{
    return clipboard->provider;
}

char* clip_clipboard_get_text(Clipboard* clipboard)
{
    return clipboard->text;
}

void clip_clipboard_set_text(Clipboard* clipboard, char* text)
{
    if(clipboard->text != NULL){
        g_free(clipboard->text);
    }
    clipboard->text = g_strdup(text);
}
