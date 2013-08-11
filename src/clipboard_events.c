/**
 * Copyright (C) 2013 Richard Burnison
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

#include "clipboard_events.h"
#include "utils.h"

#include <glib.h>

static GList *observers;

void clip_events_add_observer(void (*clip_event_listener)(ClipboardEvent event, ClipboardEntry* entry))
{
    observers = g_list_prepend(observers, clip_event_listener);
}

void clip_events_notify(ClipboardEvent event, ClipboardEntry *entry)
{
    trace("Firing event %d for %"PRIu64".\n", event, clip_clipboard_entry_get_id(entry));
    GList *next = g_list_first(observers);
    while(next != NULL){
        void (*func)() = next->data;
        func(event, entry);
        next = g_list_next(next);
    }
}
