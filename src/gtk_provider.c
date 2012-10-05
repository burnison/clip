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
#include "utils.h"

#include <gtk/gtk.h>

struct provider {
	GtkClipboard* clipboard;
	GtkClipboard* primary;
};

ClipboardProvider* clip_provider_new(void)
{
	ClipboardProvider* provider = g_malloc(sizeof(ClipboardProvider));
	provider->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	provider->primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	return provider;
}

void clip_provider_free(ClipboardProvider* provider)
{
    if(provider == NULL){
        return;
    }
    provider->clipboard = NULL;
    provider->primary = NULL;
	g_free(provider);
}


static char* clip_provider_get_clipboard(ClipboardProvider* provider)
{
	return gtk_clipboard_wait_for_text(provider->clipboard);
}

static char* clip_provider_get_primary(ClipboardProvider* provider)
{
	return gtk_clipboard_wait_for_text(provider->primary);
}


static char* clip_provider_get_synced(ClipboardProvider* provider)
{
	char* on_clipboard = clip_provider_get_clipboard(provider);
	char* on_primary = clip_provider_get_primary(provider);
	char* selection = NULL;

	// If primary is set and the two clipboards aren't synced, use primary.
	if(on_primary != NULL && g_strcmp0(on_clipboard, on_primary)){
		selection = on_primary;
	} else {
		selection = on_clipboard;
	}

	selection = g_strdup(selection);
	g_free(on_clipboard);
	g_free(on_primary);
    return selection;
}


char* clip_provider_get_current(ClipboardProvider* provider)
{
	return clip_provider_get_synced(provider);
}

void clip_provider_free_current(char* current)
{
	g_free(current);
}

/**
 * Updates the clipboard only if the values are textually different. If the active X11 selection is changed (even with
 * the same string), the current selection is unhilighted. As such, only swap the two values if they are actually
 * different.
 * <p>
 * It's possible that once this method has completed, the actual data objects of all clipboards are different. This is
 * alright.
 */
static void clip_provider_set_if_different(GtkClipboard* clipboard, char* old, char* new)
{
	if(g_strcmp0(old, new)){
		gtk_clipboard_set_text(clipboard, new, -1);
	}
}

void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
    char* current = clip_provider_get_synced(provider);

	clip_provider_set_if_different(provider->clipboard, current, text);
	clip_provider_set_if_different(provider->primary, current, text);

    g_free(current);
}

void clip_provider_clear(ClipboardProvider* provider)
{
    trace("Clearing clipboards.\n");
    // For some reason, the clear call isn't good enough. It has to be set to an empty string.
    gtk_clipboard_set_text(provider->primary, "", -1);
    gtk_clipboard_set_text(provider->clipboard, "", -1);

    gtk_clipboard_clear(provider->clipboard);
    gtk_clipboard_clear(provider->primary);
}
