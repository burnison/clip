#include "clipboard.h"

#include "utils.h"

#include <gtk/gtk.h>


struct clipboard {
    char* current;
};


Clipboard* clip_clipboard_new()
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->current = g_malloc(0);

    return clipboard;
}

void clip_clipboard_free(Clipboard* clipboard)
{
    if(clipboard == NULL){
        return;
    }

    g_free(clipboard->current);
    g_free(clipboard);
}

char* clip_clipboard_get_head(Clipboard* clipboard)
{
    return clipboard->current;
}

void clip_clipboard_set_head(Clipboard* clipboard, char* text)
{
    g_free(clipboard->current);

    clipboard->current = g_strdup(text);
}
