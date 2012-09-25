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
    clipboard->history = NULL;

    g_free(clipboard->active);
    clipboard->active = NULL;

    clipboard->provider = NULL;

    g_free(clipboard);
}



void clip_clipboard_clear(Clipboard* clipboard)
{
    g_list_free_full(clipboard->history, (GDestroyNotify)g_free);
    clipboard->history = NULL;

    g_free(clipboard->active);
    clipboard->active = NULL;

    clip_provider_clear(clipboard->provider);
}



static void clip_clipboard_remove_from_history(Clipboard* clipboard, GList* entry)
{
    char* data = entry->data;
    clipboard->history = g_list_remove(clipboard->history, data);
    g_free(data);
}

static void clip_clipboard_remove_from_history_if_exists(Clipboard* clipboard, char* text)
{
    GList* list = g_list_first(clipboard->history);
    while((list = g_list_next(list))){
        if(!g_strcmp0(list->data, text)){
            clip_clipboard_remove_from_history(clipboard, list);
            break;
        }
    }
}


void clip_clipboard_set_active(Clipboard* clipboard, char* text)
{
    trace("Setting new active clipboard value.\n");

    // If the active value is the same, just return.
    if(!g_strcmp0(clipboard->active, text)){
        return;
    }

    // Set the new value.
    g_free(clipboard->active);
    clipboard->active = g_strdup(text);

    // Remove the historical value, if it exists, and then prepend it.
    clip_clipboard_remove_from_history_if_exists(clipboard, text);
    // The previously-invoked remove_from_history function performs the free on this alloc.
    char* data = g_strdup(text);
    clipboard->history = g_list_prepend(clipboard->history, data);

    // There are more entries than allowable. Remove the tail.
    if(g_list_length(clipboard->history) > MAX_MENU_ENTRIES){
        GList* last = g_list_last(clipboard->history);
        clip_clipboard_remove_from_history(clipboard, last);
    }

    clip_provider_set_current(clipboard->provider, text);
}

char* clip_clipboard_get_active(Clipboard* clipboard)
{
    return g_strdup(clipboard->active);
}

void clip_clipboard_free_active(char* text)
{
    g_free(text);
}



GList* clip_clipboard_get_history(Clipboard* clipboard)
{
    return g_list_copy(clipboard->history);
}

void clip_clipboard_free_history(GList* history)
{
    g_list_free(history);
}
