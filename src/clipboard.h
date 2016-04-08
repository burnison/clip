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
#include "provider.h"

#include <glib.h>

#ifndef __CLIP_CLIPBOARD_TYPE_H__
#define __CLIP_CLIPBOARD_TYPE_H__
typedef enum { TRIM_OFF, TRIM_CHOMP, TRIM_CHUG, TRIM_STRIP,     TRIM_STOP } TrimMode;
#endif

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider *provider);
void clip_clipboard_free(Clipboard *clipboard);


/**
 * Gets a copy of the current clipboard's contents. This copy must be
 * freed when no longer used.
 */
ClipboardEntry* clip_clipboard_get(Clipboard *clipboard);
/**
 * Gets a copy of the clipboard's inactive contents. This copy must be
 * freed whe no longer in use.
 */
ClipboardEntry* clip_clipboard_get_head(Clipboard *clipboard);
/**
 * Identifies if the specified entry is the current entry of the clipboard.
 */
gboolean clip_clipboard_is_head(Clipboard *clipboard, ClipboardEntry *entry);

gboolean clip_clipboard_is_synced_with_provider(Clipboard *clipboard);
void clip_clipboard_sync_with_provider(Clipboard *clipboard);


/**
 * Sets the clipboard's current value to a copy of the current entry. This has
 * the same characteristics as set_new. Sometimes, Clip thinks it knows best
 * whether or not the clipboard should be set (say, perhaps, the shift key
 * is still depressed and PRIMARY is used as a source). In instances like these,
 * you can force a set regardless of how smart Clip is.
 */
void clip_clipboard_set(Clipboard *clipboard, ClipboardEntry *entry, gboolean force);
/**
 * Sets the clipboards current value to a copy of the specified text. If text is
 * NULL, the current clipboard value, along with its associated history entry,
 * are purged.
 */
void clip_clipboard_set_new(Clipboard *clipboard, char *text);


/**
 * Remove the specified entry.
 */
gboolean clip_clipboard_remove(Clipboard *clipboard, ClipboardEntry *entry);
/**
 * Save the changes to the specified entry without changing the current 
 * clipboard value.
 */
gboolean clip_clipboard_replace(Clipboard *clipboard, ClipboardEntry *entry);
/**
 * Clear the clipboard.
 */
void clip_clipboard_clear(Clipboard *clipboard);





/**
 * Join the specified entry with its next adjacent entry. This will not modify 
 * the clipboard's current state.
 */
gboolean clip_clipboard_join(Clipboard *clipboard, ClipboardEntry *left);
gboolean clip_clipboard_trim(Clipboard *clipboard, ClipboardEntry *entry);
gboolean clip_clipboard_to_upper(Clipboard *clipboard, ClipboardEntry *entry);
gboolean clip_clipboard_to_lower(Clipboard *clipboard, ClipboardEntry *entry);
gboolean clip_clipboard_toggle_lock(Clipboard *clipboard, ClipboardEntry *entry);
gboolean clip_clipboard_toggle_mask(Clipboard *clipboard, ClipboardEntry *entry);
gboolean clip_clipboard_tag(Clipboard *clipboard, ClipboardEntry *entry, char tag);


gboolean clip_clipboard_is_enabled(Clipboard *clipboard);
gboolean clip_clipboard_toggle_history(Clipboard *clipboard);
void clip_clipboard_enable_history(Clipboard *clipboard);
void clip_clipboard_disable_history(Clipboard *clipboard);


TrimMode clip_clipboard_next_trim_mode(Clipboard *clipboard);
TrimMode clip_clipboard_get_trim_mode(Clipboard *clipboard);


GList* clip_clipboard_get_history(Clipboard *clipboard);
void clip_clipboard_free_history(GList *history);
