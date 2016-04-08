/*
 * Copyright (c) 2016 Richard Burnison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "clipboard_entry.h"

struct clipboard_entry {
    uint64_t id;
    char *text;
    unsigned int count;
    char tag;
    gboolean locked;
    gboolean masked;
};


ClipboardEntry* clip_clipboard_entry_new(int64_t id, char *text, gboolean locked, unsigned int count, char tag, gboolean masked)
{
    ClipboardEntry *entry = g_malloc(sizeof(ClipboardEntry));
    entry->id = id;
    entry->text = g_strdup(text);
    entry->locked = locked;
    entry->count = count;
    entry->tag = tag;
    entry->masked = masked;
    return entry;
}

ClipboardEntry* clip_clipboard_entry_clone(ClipboardEntry *entry)
{
    if(entry == NULL){
        return NULL;
    }
    return clip_clipboard_entry_new(entry->id, entry->text, entry->locked, entry->count, entry->tag, entry->masked);
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


char clip_clipboard_entry_get_tag(ClipboardEntry *entry)
{
    return entry->tag;
}

gboolean clip_clipboard_entry_has_tag(ClipboardEntry *entry, char tag)
{
    if(entry->tag == 0){
        return FALSE;
    }
    return entry->tag == tag;
}

void clip_clipboard_entry_set_tag(ClipboardEntry *entry, char tag)
{
    entry->tag = tag;
}

void clip_clipboard_entry_remove_tag(ClipboardEntry *entry)
{
    entry->tag = 0;
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

gboolean clip_clipboard_entry_same(ClipboardEntry *a, ClipboardEntry *b)
{
    if(a == b) {
        return TRUE;
    } else if(a == NULL || b == NULL) {
        return FALSE;
    }
    return a->id == b->id;
}

gboolean clip_clipboard_entry_is_masked(ClipboardEntry *entry)
{
    return entry->masked;
}

void clip_clipboard_entry_set_masked(ClipboardEntry *entry, gboolean masked)
{
    entry->masked = masked;
}
