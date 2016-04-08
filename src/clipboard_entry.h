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

#include <glib.h>
#include <inttypes.h>

typedef struct clipboard_entry ClipboardEntry;

ClipboardEntry* clip_clipboard_entry_new(int64_t id, char *text, gboolean locked, unsigned int count, char tag, gboolean masked);
ClipboardEntry* clip_clipboard_entry_clone(ClipboardEntry *entry);
void clip_clipboard_entry_free(ClipboardEntry *entry);

gboolean clip_clipboard_entry_is_new(ClipboardEntry *entry);

uint64_t clip_clipboard_entry_get_id(ClipboardEntry *entry);
void clip_clipboard_entry_set_id(ClipboardEntry *entry, uint64_t id);

/**
 * Return a pointer to the entry's text.
 */
char* clip_clipboard_entry_get_text(ClipboardEntry *entry);
/**
 * Change the entry's text.
 */
void clip_clipboard_entry_set_text(ClipboardEntry *entry, char *text);

char clip_clipboard_entry_get_tag(ClipboardEntry *entry);
gboolean clip_clipboard_entry_has_tag(ClipboardEntry *entry, char tag);
void clip_clipboard_entry_set_tag(ClipboardEntry *entry, char tag);
void clip_clipboard_entry_remove_tag(ClipboardEntry *entry);

unsigned int clip_clipboard_entry_get_count(ClipboardEntry *entry);

gboolean clip_clipboard_entry_get_locked(ClipboardEntry *entry);
void clip_clipboard_entry_set_locked(ClipboardEntry *entry, gboolean locked);

/**
 * Determines if the two entries are equal by ID or value.
 */
gboolean clip_clipboard_entry_equals(ClipboardEntry *a, ClipboardEntry *b);
/**
 * Determines if the two entries are the same entry (by ID).
 */
gboolean clip_clipboard_entry_same(ClipboardEntry *a, ClipboardEntry *b);

gboolean clip_clipboard_entry_is_masked(ClipboardEntry *entry);
void clip_clipboard_entry_set_masked(ClipboardEntry *entry, gboolean masked);
