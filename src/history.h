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

typedef struct history ClipboardHistory;
typedef struct entry ClipboardHistoryEntry;


ClipboardHistory* clip_history_new(void);
void clip_history_free(ClipboardHistory* history);


char* clip_history_entry_get_text(ClipboardHistoryEntry* entry);
void clip_history_entry_free_text(char* text);

gboolean clip_history_entry_get_locked(ClipboardHistoryEntry* entry);
void clip_history_entry_toggle_lock(ClipboardHistory* history, ClipboardHistoryEntry* entry);

void clip_history_prepend(ClipboardHistory* history, char* text);
void clip_history_remove(ClipboardHistory* history, ClipboardHistoryEntry* entry);
void clip_history_remove_head(ClipboardHistory* history);

void clip_history_clear(ClipboardHistory* history);

GList* clip_history_get_list(ClipboardHistory* history);
void clip_history_free_list(GList* list);
