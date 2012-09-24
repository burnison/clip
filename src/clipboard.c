#include "clipboard.h"

#include "utils.h"

#include <glib.h>


struct clipboard {
    ClipboardProvider* provider;
    GList* history;
    char* active;
};


Clipboard* clip_clipboard_new(ClipboardProvider* provider)
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->history = NULL;
    clipboard->active = NULL;
    clipboard->provider = provider;

    return clipboard;
}

void clip_clipboard_free(Clipboard* clipboard)
{
    if(clipboard == NULL){
        return;
    }

    g_list_free_full(clipboard->history, g_free);
    g_free(clipboard->active);
    clipboard->provider = NULL;

    g_free(clipboard);
}



static void clip_clipboard_remove_from_history(Clipboard* clipboard, char* text)
{
    GList* list = g_list_first(clipboard->history);
    while((list = g_list_next(list))){
        if(!g_strcmp0(list->data, text)){
            debug("New clipboard value exists in history. Promoting.\n");

            char* data = list->data;
            clipboard->history = g_list_remove(clipboard->history, data);

            // This is the only place historical data gets freed.
            g_free(data);
            break;
        }
    }
}

char* clip_clipboard_get_active(Clipboard* clipboard)
{
    return clipboard->active;
}

void clip_clipboard_set_active(Clipboard* clipboard, char* text)
{
    trace("Setting new active clipboard value.\n");

    // If the active value is the same, just return.
    if(!g_strcmp0(clipboard->active, text)){
        return;
    }

    clip_clipboard_remove_from_history(clipboard, text);

    char* copy = g_strdup(text);
    clipboard->active = copy;
    clipboard->history = g_list_prepend(clipboard->history, copy);

    clip_provider_set_current(clipboard->provider, copy);
}



GList* clip_clipboard_get_history(Clipboard* clipboard)
{
    return g_list_copy(clipboard->history);
}

void clip_clipboard_free_history(GList* history)
{
    g_list_free(history);
}
