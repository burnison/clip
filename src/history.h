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

#include "clipboard_entry.h"

#include <glib.h>

typedef struct history ClipboardHistory;

ClipboardHistory* clip_history_new(void);
void clip_history_free(ClipboardHistory *history);
void clip_history_free_list(GList *list);

gboolean clip_history_prepend(ClipboardHistory *history, ClipboardEntry *entry);
gboolean clip_history_update(ClipboardHistory *history, ClipboardEntry *entry);

gboolean clip_history_remove(ClipboardHistory *history, ClipboardEntry *entry);
gboolean clip_history_remove_head(ClipboardHistory *history);
void clip_history_clear(ClipboardHistory *history);

GList* clip_history_get_list(ClipboardHistory *history);
ClipboardEntry* clip_history_get_head(ClipboardHistory *history);
ClipboardEntry* clip_history_get_similar(ClipboardHistory *history, ClipboardEntry *entry, int limit_scan);

