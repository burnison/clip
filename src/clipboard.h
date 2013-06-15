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

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider* provider);
void clip_clipboard_free(Clipboard* clipboard);


ClipboardEntry* clip_clipboard_get(Clipboard* clipboard);
void clip_clipboard_set(Clipboard* clipboard, char* text);
void clip_clipboard_remove(Clipboard* clipboard, ClipboardEntry* entry);
void clip_clipboard_toggle_lock(Clipboard* clipboard, ClipboardEntry* entry);

void clip_clipboard_clear(Clipboard* clipboard);

gboolean clip_clipboard_is_enabled(Clipboard* clipboard);
gboolean clip_clipboard_toggle_history(Clipboard* clipboard);
void clip_clipboard_enable_history(Clipboard* clipboard);
void clip_clipboard_disable_history(Clipboard* clipboard);

GList* clip_clipboard_get_history(Clipboard* clipboard);
void clip_clipboard_free_history(GList* history);
