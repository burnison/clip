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
