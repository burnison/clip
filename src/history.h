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

#include <glib.h>

typedef struct history ClipboardHistory;

ClipboardHistory* clip_history_new(void);
void clip_history_free(ClipboardHistory* history);

void clip_history_prepend(ClipboardHistory* history, ClipboardEntry* entry);
void clip_history_remove(ClipboardHistory* history, ClipboardEntry* entry);
void clip_history_remove_head(ClipboardHistory* history);
void clip_history_toggle_lock(ClipboardHistory* history, ClipboardEntry* entry);

void clip_history_clear(ClipboardHistory* history);

GList* clip_history_get_list(ClipboardHistory* history);
ClipboardEntry* clip_history_get_head(ClipboardHistory* history);
void clip_history_free_list(GList* list);
