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

#include "provider.h"
#include "history.h"

#include <glib.h>

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider* provider);
void clip_clipboard_free(Clipboard* clipboard);

void clip_clipboard_set_active(Clipboard* clipboard, char* text);
char* clip_clipboard_get_active(Clipboard* clipboard);
void clip_clipboard_free_active(char* text);

// Enables/disables this clipboard's history. Enabling will force a flush.
void clip_clipboard_toggle(Clipboard* clipboard);
gboolean clip_clipboard_is_enabled(Clipboard* clipboard);


// History delegates follow.
void clip_clipboard_clear(Clipboard* clipboard);
void clip_clipboard_remove(Clipboard* clipboard, ClipboardHistoryEntry* entry);
void clip_clipboard_toggle_lock(Clipboard* clipboard, ClipboardHistoryEntry* entry);

GList* clip_clipboard_get_history(Clipboard* clipboard);
void clip_clipboard_free_history(GList* history);
