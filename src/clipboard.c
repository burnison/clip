/**
 * Copyright (C) 2012 Richard Burnison
 *
 * This file is part of Clip.
 *
 * Clip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Clip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Clip.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "clipboard.h"
#include "history.h"
#include "utils.h"

#include <glib.h>


struct clipboard {
    ClipboardProvider* provider;
    ClipboardHistory* history;
    ClipboardEntry* current;
    gboolean enabled;
};



Clipboard* clip_clipboard_new(ClipboardProvider* provider)
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->provider = provider;
    clipboard->history = clip_history_new();
    clipboard->enabled = TRUE;
    clipboard->current = NULL;
    return clipboard;
}

void clip_clipboard_free(Clipboard* clipboard)
{
    if(clipboard == NULL){
        return;
    }

    clip_history_free(clipboard->history);
    clipboard->history = NULL;

    clip_clipboard_entry_free(clipboard->current);
    clipboard->current = NULL;

    clipboard->provider = NULL;

    g_free(clipboard);
}



/**
 * Sets the clipboard's current value to a copy of the specified text. If text is NULL, the current clipboard value,
 * along with its associated history entry, are purged.
 */
void clip_clipboard_set(Clipboard* clipboard, char* text)
{
    debug("Setting new active clipboard value, \"%.*s...\".\n", 30, text);

    // If the active value is the same, just return.
    if(clipboard->current != NULL){
        char* current = clip_clipboard_entry_get_text(clipboard->current);
        if(!g_strcmp0(current, text)){
            trace("Clipboard's active text matches new text. Ignoring.\n");
            return;
        } 
    }

    clip_provider_set_current(clipboard->provider, text);

    // Set the new value.
    clip_clipboard_entry_free(clipboard->current);
    clipboard->current = clip_clipboard_entry_new(text, FALSE, 0);

    if(clip_clipboard_is_enabled(clipboard)){
        if(text == NULL){
            debug("New clipboard contents are null (probably a request to clear the clipboard). Removing head.\n");
            clip_history_remove_head(clipboard->history);
        } else {
            clip_history_prepend(clipboard->history, clipboard->current);
        }
    }
}

/**
 * Gets a copy of the current clipboard's contents. This copy must be freed when no longer used.
 */
ClipboardEntry* clip_clipboard_get(Clipboard* clipboard)
{
    return clip_clipboard_entry_clone(clipboard->current);
}


void clip_clipboard_remove(Clipboard* clipboard, ClipboardEntry* entry)
{
    clip_history_remove(clipboard->history, entry);
}

void clip_clipboard_toggle_lock(Clipboard* clipboard, ClipboardEntry* entry)
{
    clip_history_toggle_lock(clipboard->history, entry);
    clip_clipboard_entry_set_locked(entry, !clip_clipboard_entry_get_locked(entry));
}


void clip_clipboard_clear(Clipboard* clipboard)
{
    clip_history_clear(clipboard->history);
    clip_provider_clear(clipboard->provider);

    clip_clipboard_entry_free(clipboard->current);
    clipboard->current = NULL;
}





gboolean clip_clipboard_is_enabled(Clipboard* clipboard)
{
    return clipboard->enabled;
}

gboolean clip_clipboard_toggle_history(Clipboard* clipboard)
{
    trace("Toggling clipboard.\n");
    clipboard->enabled = !clip_clipboard_is_enabled(clipboard);
    return clip_clipboard_is_enabled(clipboard);
}

void clip_clipboard_enable_history(Clipboard* clipboard)
{
    clipboard->enabled = TRUE;
}

void clip_clipboard_disable_history(Clipboard* clipboard)
{
    clipboard->enabled = FALSE;
}


GList* clip_clipboard_get_history(Clipboard* clipboard)
{
    return clip_history_get_list(clipboard->history);
}

void clip_clipboard_free_history(GList* history)
{
    clip_history_free_list(history);
}
