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

#include "clipboard_entry.h"


#ifndef __CLIP_EVENTS_TYPES__
#define __CLIP_EVENTS_TYPES__
typedef enum {CLIPBOARD_ADD_EVENT, CLIPBOARD_REMOVE_EVENT, CLIPBOARD_UPDATE_EVENT} ClipboardEvent;
#endif

void clip_events_add_observer(void (*clip_event_listener)(ClipboardEvent event, ClipboardEntry* entry));

void clip_events_notify(ClipboardEvent event, ClipboardEntry *entry);
