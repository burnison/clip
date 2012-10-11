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
    char* current;
    gboolean ownership_transferred;
    gboolean waiting;
};


void clip_provider_cb_owner_changed(GtkClipboard* clipboard, GdkEvent* event, gpointer data)
{
	if(((GdkEventOwnerChange*)event)->reason != GDK_OWNER_CHANGE_NEW_OWNER) {
        debug("Clipboard owner has been destroyed. Going to ignore next null.\n");
        ((ClipboardProvider*)data)->ownership_transferred = TRUE;
    }
}

ClipboardProvider* clip_provider_new(void)
{
	ClipboardProvider* provider = g_malloc(sizeof(ClipboardProvider));
	provider->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	provider->primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    provider->current = NULL;
    provider->ownership_transferred = FALSE;
    provider->waiting = FALSE;

    g_signal_connect(G_OBJECT(provider->clipboard), "owner-change", G_CALLBACK(clip_provider_cb_owner_changed), provider);
    g_signal_connect(G_OBJECT(provider->primary), "owner-change", G_CALLBACK(clip_provider_cb_owner_changed), provider);

	return provider;
}

void clip_provider_free(ClipboardProvider* provider)
{
    if(provider == NULL){
        return;
    }

    g_signal_handlers_disconnect_by_data(provider->clipboard, provider);
    provider->clipboard = NULL;

    g_signal_handlers_disconnect_by_data(provider->primary, provider);
    provider->primary = NULL;

    g_free(provider->current);
    provider->current = NULL;

	g_free(provider);
}


/**
 * Updates the clipboard only if the values are textually different. If the active X11 selection is changed (even with
 * the same string), the current selection is unhilighted. As such, only swap the two values if they are actually
 * different, textually.
 */
static void clip_provider_set_if_different(GtkClipboard* clipboard, char* new)
{
    char* old = gtk_clipboard_wait_for_text(clipboard);
	if(g_strcmp0(old, new)){
		gtk_clipboard_set_text(clipboard, new == NULL ? "" : new, -1);
	}
    g_free(old);
}

/**
 * Sets the specified value, text, to be the current upstream clipboard contents.
 */
void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
    if(provider->waiting){
        warn("Attempted to set upstream clipboard contents when they are already being set."
                " This is indicative of a logic bug.\n");
        return;
    }

    if(provider->ownership_transferred){
        provider->ownership_transferred = FALSE;
        if(text == NULL){
            debug("Detected an ownership transfer resulting in a clipboard purge. Going to restore old value.\n");
            char* current = g_strdup(provider->current);
            clip_provider_set_current(provider, current);
            g_free(current);
        }
        return;
    }

    provider->waiting = TRUE;
	clip_provider_set_if_different(provider->clipboard, text);
	clip_provider_set_if_different(provider->primary, text);
    provider->waiting = FALSE;

    g_free(provider->current);
    provider->current = g_strdup(text);
}

void clip_provider_clear(ClipboardProvider* provider)
{
    if(provider->waiting){
        warn("Attempted to clear current clipboard values when already operating."
                " This is indicative of a logic bug.\n");
        return;
    }

    trace("Clearing clipboards.\n");

    provider->waiting = TRUE;
    gtk_clipboard_set_text(provider->primary, "", -1);
    gtk_clipboard_set_text(provider->clipboard, "", -1);
    provider->waiting = FALSE;
}


/**
 * Identifies if the provider is able to be quried---that is, if the clipboards have properly been synced and
 * have been fully read. This method should be invoked prior to executing get_current, however, no such preconditions
 * will be enforced. This may result in stale data.
 */
gboolean clip_provider_current_available(ClipboardProvider* provider)
{
    return !provider->waiting;
}






/**
 * Remember to free the returned string.
 */
static char* clip_provider_get_synced(ClipboardProvider* provider)
{
    provider->waiting = TRUE;
	char* on_clipboard = gtk_clipboard_wait_for_text(provider->clipboard);
	char* on_primary = gtk_clipboard_wait_for_text(provider->primary);
    provider->waiting = FALSE;

	char* selection = NULL;

	// If primary is set and the two clipboards aren't synced, use primary.
	if(g_strcmp0(on_clipboard, on_primary)){
		selection = on_primary;
	} else {
		selection = on_clipboard;
	}

	selection = g_strdup(selection);
	g_free(on_clipboard);
	g_free(on_primary);

    return selection;
}

/**
 * Gets the current value of the system clipboard.
 */
char* clip_provider_get_current(ClipboardProvider* provider)
{
    if(!provider->waiting){
        char* synced = clip_provider_get_synced(provider);
        clip_provider_set_current(provider, synced);
        g_free(synced);
    }

	return g_strdup(provider->current);
}

void clip_provider_free_current(char* current)
{
	g_free(current);
}
