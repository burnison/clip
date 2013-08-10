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
#include "provider.h"

#include <glib.h>

#ifndef __CLIP_CLIPBOARD_TYPE_H__
#define __CLIP_CLIPBOARD_TYPE_H__
typedef enum { TRIM_OFF, TRIM_CHOMP, TRIM_CHUG, TRIM_STRIP,     TRIM_STOP } TrimMode;
#endif

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider* provider);
void clip_clipboard_free(Clipboard* clipboard);


/**
 * Gets a copy of the current clipboard's contents. This copy must be
 * freed when no longer used.
 */
ClipboardEntry* clip_clipboard_get(Clipboard* clipboard);
/**
 * Identifies if the specified entry is the current entry of the clipboard.
 */
gboolean clip_clipboard_is_head(Clipboard* clipboard, ClipboardEntry* entry);


/**
 * Sets the clipboard's current value to a copy of the current entry. This has
 * the same characteristics as set_new.
 */
void clip_clipboard_set(Clipboard* clipboard, ClipboardEntry* entry);
/**
 * Sets the clipboards current value to a copy of the specified text. If text is
 * NULL, the current clipboard value, along with its associated history entry,
 * are purged.
 */
void clip_clipboard_set_new(Clipboard* clipboard, char* text);

void clip_clipboard_join(Clipboard* clipboard, ClipboardEntry* left);


void clip_clipboard_remove(Clipboard* clipboard, ClipboardEntry* entry);
void clip_clipboard_toggle_lock(Clipboard* clipboard, ClipboardEntry* entry);
void clip_clipboard_clear(Clipboard* clipboard);


gboolean clip_clipboard_is_enabled(Clipboard* clipboard);
gboolean clip_clipboard_toggle_history(Clipboard* clipboard);
void clip_clipboard_enable_history(Clipboard* clipboard);
void clip_clipboard_disable_history(Clipboard* clipboard);


TrimMode clip_clipboard_next_trim_mode(Clipboard* clipboard);
TrimMode clip_clipboard_get_trim_mode(Clipboard* clipboard);


GList* clip_clipboard_get_history(Clipboard* clipboard);
void clip_clipboard_free_history(GList* history);
