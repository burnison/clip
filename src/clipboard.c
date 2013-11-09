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
#include "clipboard_events.h"
#include "history.h"
#include "utils.h"

#include <glib.h>
#include <string.h>


struct clipboard {
    ClipboardProvider *provider;
    ClipboardHistory *history;
    ClipboardEntry *current;
    char *current_text;
    TrimMode trim_mode;
    gboolean enabled;
};



static void clip_clipboard_on_event(ClipboardEvent event, ClipboardEntry* entry);

Clipboard* clip_clipboard_new(ClipboardProvider *provider)
{
    Clipboard *clipboard = g_malloc(sizeof(Clipboard));
    clipboard->provider = provider;
    clipboard->history = clip_history_new();
    clipboard->enabled = TRUE;
    clipboard->current = NULL;
    clipboard->current_text = NULL;
    clipboard->trim_mode = DEFAULT_TRIM_MODE;

    clip_events_add_observer(clip_clipboard_on_event);

    return clipboard;
}

void clip_clipboard_free(Clipboard *clipboard)
{
    if(clipboard == NULL){
        return;
    }
    clip_history_free(clipboard->history);
    clipboard->history = NULL;
    clip_clipboard_entry_free(clipboard->current);
    clipboard->current = NULL;
    clipboard->provider = NULL;
    g_free(clipboard->current_text);
    clipboard->current_text = NULL;
    g_free(clipboard);
}


static char* clip_clipboard_clean(Clipboard *clipboard, char *text)
{
    if(text == NULL){
        return NULL;
    }
    char *str = g_strdup(text);
    switch(clipboard->trim_mode) {
        case TRIM_CHOMP:
            g_strchomp(str);
            break;
        case TRIM_CHUG:
            g_strchug(str);
            break;
        case TRIM_STRIP:
            g_strstrip(str);
            break;
        case TRIM_OFF:
        case TRIM_STOP:
            break;
    }
    return str;
}


/**
 * Comparator for GLib collections.
 */
static gboolean clip_clipboard_compare_entries(ClipboardEntry *a, ClipboardEntry *b)
{
    return !clip_clipboard_entry_equals(a, b);
}

/**
 * Identifies if the two values differ in a meaningful way, regardless of 
 * left or right padding.
 */
static gboolean clip_clipboard_different(char *new, char *old)
{
    char *stripped_new = g_strstrip(g_strdup(new));
    char *stripped_old = g_strstrip(g_strdup(old));
    gboolean different = g_strcmp0(stripped_old, stripped_new);
    g_free(stripped_old);
    g_free(stripped_new);
    return different;
}

void clip_clipboard_set_new(Clipboard *clipboard, char *text)
{
    ClipboardEntry *entry = clip_clipboard_entry_new(0, text, FALSE, 0, 0);
    clip_clipboard_set(clipboard, entry);
    clip_clipboard_entry_free(entry);
}


static gboolean clip_clipboard_replace_similar(Clipboard *clipboard, ClipboardEntry *entry)
{
    ClipboardEntry *similar = clip_history_get_similar(clipboard->history, entry, SIMILARITY_REPLACEMENT_LIMIT);
    if(similar != NULL){
        debug("Replacing similar entry, %"PRIu64".\n", clip_clipboard_entry_get_id(similar));
        char *new_text = clip_clipboard_entry_get_text(entry);
        clip_clipboard_entry_set_text(similar, new_text);
        if(!clip_clipboard_entry_is_new(entry)){
            // Just in case this is an update, remove the old one.
            clip_clipboard_remove(clipboard, entry);
        }
        clip_clipboard_set(clipboard, similar);
        clip_clipboard_entry_free(similar);
        return TRUE;
    }
    return FALSE;
}

void clip_clipboard_set(Clipboard *clipboard, ClipboardEntry *entry)
{
    if(clip_clipboard_replace_similar(clipboard, entry)){
        debug("This value was swapped out with a similar entry.\n");
        return;
    }

    char *new = clip_clipboard_entry_get_text(entry);
    char *clean_new = clip_clipboard_clean(clipboard, new);

    char *current = clip_clipboard_entry_get_text(clipboard->current);
    char *clean_current = clip_clipboard_clean(clipboard, current);

    if(clean_new != NULL && strlen(clean_new) < 1){
        debug("String is 0 characetrs long. Dropping and reverting current head.\n");
        if(clean_current == NULL || strlen(clean_current) < 1){
            clip_history_remove_head(clipboard->history);
            ClipboardEntry *head = clip_history_get_head(clipboard->history);
            if(head == NULL){
                debug("No usable values.\n");
            } else {
                debug("Current value not usable. Resetting to previous head.\n");
                clip_clipboard_set(clipboard, head);
                clip_clipboard_entry_free(head);
            }
        } else {
            clip_provider_set_current(clipboard->provider, clean_current);
        }
        goto exit;
    } else if(clean_current != NULL && !clip_clipboard_different(clean_new, clean_current)){
        // There isn't any need to archive this in history; it's the same value.
        clip_provider_set_current(clipboard->provider, clean_current);
        goto exit;
    }

    clip_provider_set_current(clipboard->provider, clean_new);


    ClipboardEntry *existing = clipboard->current;
    clipboard->current = clip_clipboard_entry_clone(entry);
    clip_clipboard_entry_free(existing);

    char *existing_text = clipboard->current_text;
    clipboard->current_text = g_strdup(clean_new);
    g_free(existing_text);

    if(clip_clipboard_is_enabled(clipboard)){
        if(new == NULL){
            debug("New clipboard contents are null (probably a request to clear the clipboard). Removing head.\n");
            clip_history_remove_head(clipboard->history);
        } else {
            debug("Setting new active clipboard value, \"%.*s...\".\n", 30, new);
            clip_history_prepend(clipboard->history, clipboard->current);
        }
    }
exit:
    g_free(clean_current);
    g_free(clean_new);
}



gboolean clip_clipboard_join(Clipboard *clipboard, ClipboardEntry *left)
{
    gboolean changed = FALSE;
    GList *list = clip_history_get_list(clipboard->history);
    GList *first = g_list_find_custom(list, left, (GCompareFunc)clip_clipboard_compare_entries);
    GList *next = g_list_next(first);
    if(first == NULL || next == NULL){
        debug("Not enough entries to join.\n");
        goto exit;
    }

    ClipboardEntry *right = next->data;
    GString *joined_text = g_string_new(clip_clipboard_entry_get_text(left));
    g_string_append_printf(joined_text, " %s", clip_clipboard_entry_get_text(right));
    clip_clipboard_entry_set_text(left, joined_text->str);
    g_string_free(joined_text, TRUE);

    clip_clipboard_remove(clipboard, right);
    clip_clipboard_replace(clipboard, left);
    changed = TRUE;

exit:
    clip_history_free_list(list);
    return changed;
}

gboolean clip_clipboard_trim(Clipboard *clipboard, ClipboardEntry *entry)
{
    char *current = clip_clipboard_entry_get_text(entry);
    g_strstrip(current);
    return clip_clipboard_replace(clipboard, entry);
}

gboolean clip_clipboard_to_upper(Clipboard *clipboard, ClipboardEntry *entry)
{
    char *current = clip_clipboard_entry_get_text(entry);
    char *lower = g_utf8_strup(current, -1);
    clip_clipboard_entry_set_text(entry, lower);
    g_free(lower);

    gboolean success = clip_clipboard_replace(clipboard, entry);
    if(!success){
        clip_clipboard_entry_set_text(entry, current);
    }
    return success;
}

gboolean clip_clipboard_to_lower(Clipboard *clipboard, ClipboardEntry *entry)
{
    char *current = clip_clipboard_entry_get_text(entry);
    char *upper = g_utf8_strdown(current, -1);
    clip_clipboard_entry_set_text(entry, upper);
    g_free(upper);

    gboolean success = clip_clipboard_replace(clipboard, entry);
    if(!success){
        clip_clipboard_entry_set_text(entry, current);
    }
    return success;
}



ClipboardEntry* clip_clipboard_get(Clipboard *clipboard)
{
    return clip_clipboard_entry_clone(clipboard->current);
}

ClipboardEntry* clip_clipboard_get_head(Clipboard *clipboard)
{
    return clip_history_get_head(clipboard->history);
}

gboolean clip_clipboard_is_head(Clipboard *clipboard, ClipboardEntry *entry)
{
    return !g_strcmp0(clip_clipboard_entry_get_text(clipboard->current), clip_clipboard_entry_get_text(entry));
}

gboolean clip_clipboard_is_synced_with_provider(Clipboard *clipboard)
{
    char *provider_contents = clip_provider_get_current(clipboard->provider);
    gboolean synced = !g_strcmp0(clipboard->current_text, provider_contents);
    clip_provider_free_current(provider_contents);
    return synced;
}

void clip_clipboard_sync_with_provider(Clipboard *clipboard)
{
    char *provider_contents = clip_provider_get_current(clipboard->provider);
    clip_clipboard_set_new(clipboard, provider_contents);
    clip_provider_free_current(provider_contents);
}


TrimMode clip_clipboard_next_trim_mode(Clipboard *clipboard)
{
    clipboard->trim_mode++;
    if(clipboard->trim_mode >= TRIM_STOP){
        clipboard->trim_mode = TRIM_OFF;
    }
    return clipboard->trim_mode;
}

TrimMode clip_clipboard_get_trim_mode(Clipboard *clipboard)
{
    return clipboard->trim_mode;
}



gboolean clip_clipboard_replace(Clipboard *clipboard, ClipboardEntry *entry)
{
    return clip_history_update(clipboard->history, entry);
}

gboolean clip_clipboard_remove(Clipboard *clipboard, ClipboardEntry *entry)
{
    return clip_history_remove(clipboard->history, entry);
}



gboolean clip_clipboard_toggle_lock(Clipboard *clipboard, ClipboardEntry *entry)
{
    gboolean locked = clip_clipboard_entry_get_locked(entry);
    clip_clipboard_entry_set_locked(entry, !locked);
    return clip_history_update(clipboard->history, entry);
}


gboolean clip_clipboard_tag(Clipboard *clipboard, ClipboardEntry *entry, char tag)
{
    if(clip_clipboard_entry_get_tag(entry) == tag) {
        clip_clipboard_entry_remove_tag(entry);
    } else {
        clip_clipboard_entry_set_tag(entry, tag);
    }
    return clip_history_update(clipboard->history, entry);
}


void clip_clipboard_clear(Clipboard *clipboard)
{
    clip_history_clear(clipboard->history);
    clip_provider_clear(clipboard->provider);

    clip_clipboard_entry_free(clipboard->current);
    clipboard->current = NULL;
}




gboolean clip_clipboard_is_enabled(Clipboard *clipboard)
{
    return clipboard->enabled;
}

gboolean clip_clipboard_toggle_history(Clipboard *clipboard)
{
    trace("Toggling clipboard.\n");
    clipboard->enabled = !clip_clipboard_is_enabled(clipboard);
    return clip_clipboard_is_enabled(clipboard);
}




void clip_clipboard_enable_history(Clipboard *clipboard)
{
    clipboard->enabled = TRUE;
}

void clip_clipboard_disable_history(Clipboard *clipboard)
{
    clipboard->enabled = FALSE;
}




GList* clip_clipboard_get_history(Clipboard *clipboard)
{
    return clip_history_get_list(clipboard->history);
}

void clip_clipboard_free_history(GList *history)
{
    clip_history_free_list(history);
}


static void clip_clipboard_on_event(ClipboardEvent event, ClipboardEntry* entry)
{
}
