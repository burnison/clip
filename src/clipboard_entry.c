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

#include "clipboard_entry.h"


struct clipboard_entry {
    uint64_t id;
    char *text;
    gboolean locked;
    unsigned int count;
};


ClipboardEntry* clip_clipboard_entry_new(int64_t id, char *text, gboolean locked, unsigned int count)
{
    ClipboardEntry *entry = g_malloc(sizeof(ClipboardEntry));
    entry->id = id;
    entry->text = g_strdup(text);
    entry->locked = locked;
    entry->count = count;
    return entry;
}

ClipboardEntry* clip_clipboard_entry_clone(ClipboardEntry *entry)
{
    if(entry == NULL){
        return NULL;
    }
    return clip_clipboard_entry_new(entry->id, entry->text, entry->locked, entry->count);
}

void clip_clipboard_entry_free(ClipboardEntry *entry)
{
    if(entry == NULL){
        return;
    }
    g_free(entry->text);
    g_free(entry);
}


gboolean clip_clipboard_entry_is_new(ClipboardEntry *entry)
{
    return entry->id == 0;
}

char* clip_clipboard_entry_get_text(ClipboardEntry *entry)
{
    if(entry == NULL){
        return NULL;
    }
    entry->count++;
    return entry->text;
}

void clip_clipboard_entry_set_text(ClipboardEntry *entry, char *text)
{
    if(entry == NULL){
        return;
    }
    char *old_text = entry->text;
    entry->text = g_strdup(text);
    g_free(old_text);
}

uint64_t clip_clipboard_entry_get_id(ClipboardEntry *entry)
{
    if(entry == NULL){
        return 0;
    }
    return entry->id;
}

void clip_clipboard_entry_set_id(ClipboardEntry *entry, uint64_t id)
{
    entry->id = id;
}

unsigned int clip_clipboard_entry_get_count(ClipboardEntry *entry)
{
    if(entry == NULL){
        return -1;
    }
    return entry->count;
}


gboolean clip_clipboard_entry_get_locked(ClipboardEntry *entry)
{
    if(entry == NULL){
        return FALSE;
    }
    return entry->locked;
}
void clip_clipboard_entry_set_locked(ClipboardEntry *entry, gboolean locked)
{
    entry->locked = locked;
}



gboolean clip_clipboard_entry_equals(ClipboardEntry *a, ClipboardEntry *b)
{
    if(a == b) {
        return TRUE;
    } else if(a == NULL || b == NULL) {
        return FALSE;
    }
    return a->id == b->id || !g_strcmp0(a->text, b->text);
}
