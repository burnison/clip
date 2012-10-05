#include "clipboard.h"

#include "history.h"
#include "utils.h"

#include <glib.h>


struct clipboard {
    ClipboardProvider* provider;
    ClipboardHistory* history;
    gboolean enabled;
    char* active;
};



Clipboard* clip_clipboard_new(ClipboardProvider* provider)
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->provider = provider;
    clipboard->history = clip_history_new();
    clipboard->enabled = TRUE;
    clipboard->active = NULL;

    return clipboard;
}

void clip_clipboard_free(Clipboard* clipboard)
{
    if(clipboard == NULL){
        debug("Attempted to free NULL clipboard.\n");
        return;
    }

    clip_history_free(clipboard->history);
    clipboard->history = NULL;

    g_free(clipboard->active);
    clipboard->active = NULL;

    clipboard->provider = NULL;

    g_free(clipboard);
}


void clip_clipboard_set_active(Clipboard* clipboard, char* text)
{
    trace("Setting new active clipboard value, \"%.*s...\".\n", 30, text);

    // If the active value is the same, just return.
    if(!g_strcmp0(clipboard->active, text)){
        trace("Clipboard's active text matches new text. Ignoring.\n");
        return;
    }

    // Set the new value.
    g_free(clipboard->active);
    clipboard->active = g_strdup(text);

    clip_provider_set_current(clipboard->provider, text);

    if(clip_clipboard_is_enabled(clipboard)){
        clip_history_prepend(clipboard->history, text);
    } else {
        debug("Clipboard is disabled. Not appending to history.\n");
    }
}

char* clip_clipboard_get_active(Clipboard* clipboard)
{
    return g_strdup(clipboard->active);
}

void clip_clipboard_free_active(char* text)
{
    g_free(text);
}






gboolean clip_clipboard_is_enabled(Clipboard* clipboard)
{
    return clipboard->enabled;
}

void clip_clipboard_toggle(Clipboard* clipboard)
{
    trace("Toggling clipboard.\n");
    clipboard->enabled = !clip_clipboard_is_enabled(clipboard);
}



void clip_clipboard_clear(Clipboard* clipboard)
{
    clip_history_clear(clipboard->history);
    clip_provider_clear(clipboard->provider);

    g_free(clipboard->active);
    clipboard->active = NULL;
}

void clip_clipboard_remove(Clipboard* clipboard, ClipboardHistoryEntry* entry)
{
    clip_history_remove(clipboard->history, entry);
}

void clip_clipboard_toggle_lock(Clipboard* clipboard, ClipboardHistoryEntry* entry)
{
    clip_history_entry_toggle_lock(clipboard->history, entry);
}



GList* clip_clipboard_get_history(Clipboard* clipboard)
{
    return clip_history_get_list(clipboard->history);
}

void clip_clipboard_free_history(GList* history)
{
    clip_history_free_list(history);
}
