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

#include <glib.h>
#include <inttypes.h>

typedef struct clipboard_entry ClipboardEntry;

ClipboardEntry* clip_clipboard_entry_new(int64_t id, char* text, gboolean locked, unsigned int count);
ClipboardEntry* clip_clipboard_entry_clone(ClipboardEntry* entry);
void clip_clipboard_entry_free(ClipboardEntry* entry);

gboolean clip_clipboard_entry_is_new(ClipboardEntry* entry);

uint64_t clip_clipboard_entry_get_id(ClipboardEntry* entry);
void clip_clipboard_entry_set_id(ClipboardEntry* entry, uint64_t id);

/**
 * Return a pointer to the entry's text.
 */
char* clip_clipboard_entry_get_text(ClipboardEntry* entry);
/**
 * Change the entry's text.
 */
void clip_clipboard_entry_set_text(ClipboardEntry* entry, char* text);

unsigned int clip_clipboard_entry_get_count(ClipboardEntry* entry);

gboolean clip_clipboard_entry_get_locked(ClipboardEntry* entry);
void clip_clipboard_entry_set_locked(ClipboardEntry* entry, gboolean locked);

gboolean clip_clipboard_entry_equals(ClipboardEntry* a, ClipboardEntry* b);
