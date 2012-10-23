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
#include <string.h>

struct provider {
	GtkClipboard* clipboard;
	GtkClipboard* primary;
    char* current;
    gboolean ownership_transferred;
    gboolean locked;
};


static void clip_provider_unlock(ClipboardProvider* provider)
{
    provider->locked = FALSE;
}


void clip_provider_cb_owner_changed(GtkClipboard* clipboard, GdkEvent* event, gpointer data)
{
	if(((GdkEventOwnerChange*)event)->reason != GDK_OWNER_CHANGE_NEW_OWNER) {
        debug("Clipboard owner has been destroyed. Going to ignore next value if null.\n");
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
    provider->locked = FALSE;

    g_signal_connect(G_OBJECT(provider->primary), "owner-change", G_CALLBACK(clip_provider_cb_owner_changed), provider);
    g_signal_connect(G_OBJECT(provider->clipboard), "owner-change",
            G_CALLBACK(clip_provider_cb_owner_changed), provider);

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
 * When requesting content from a GTK clipboard, the actual thread is interleaved, returning control back to the GTK
 * main loop (via recursion). As such, it's possible a the current wait may be pre-empted by a subsequent wait,
 * resulting in potentially indeterminate ordering. To prevent the first wait from being pre-empted, this function
 * serves as a primitive mutex on the provider.
 * @return TRUE if the lock has been acquired, FALSE otherwise.
 */
static gboolean clip_provider_lock(ClipboardProvider* provider)
{
    if(provider->locked){
        return FALSE;
    }
    provider->locked = TRUE;
    return TRUE;
}

/**
 * Determines if the clipboards are in a state wherein content can be used. For example, if the right mouse button is
 * currently pressed, we can assume that the primary clipboard is not fully complete, and thus, the content is not yet
 * ready to be consumed.
 */
static gboolean clip_provider_is_provider_ready()
{
    GdkWindow* root_window = gdk_get_default_root_window();
    GdkDeviceManager* device_manager = gdk_display_get_device_manager(gdk_display_get_default());
    GdkDevice* pointer = gdk_device_manager_get_client_pointer(device_manager);
    GdkModifierType modifiers = {0};

    gdk_window_get_device_position(root_window, pointer, NULL, NULL, &modifiers);

    if(modifiers & (GDK_BUTTON1_MASK | GDK_SHIFT_MASK)){
        return FALSE;
    }
    return TRUE;
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

static char* clip_provider_prepare_value(ClipboardProvider* provider, char* text)
{
    if(provider->ownership_transferred){
        provider->ownership_transferred = FALSE;
        if(text == NULL){
            debug("Encountered a null after ownership transfer. Dropping and reverting current head.\n");
            return g_strdup(provider->current);
        }
    }

    char* copy = g_strdup(text);

    if(copy != NULL && PROVIDER_TRIM){
        g_strchomp(copy);
        if(strlen(copy) < 1){
            debug("Trimmed string is 0 characetrs long. Dropping and reverting current head.\n");
            g_free(copy);
            return g_strdup(provider->current);
        }
    }

    return copy;
}

/**
 * Sets the provider clipboards to a copy of the specified value.
 */
void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
    if(!clip_provider_is_provider_ready()){
        return;
    }

    char* copy = clip_provider_prepare_value(provider, text);
    if(!clip_provider_lock(provider)){
        g_free(copy);
        return;
    }
	clip_provider_set_if_different(provider->clipboard, copy);
	clip_provider_set_if_different(provider->primary, copy);
    clip_provider_unlock(provider);

    g_free(provider->current);
    provider->current = copy;
}

void clip_provider_clear(ClipboardProvider* provider)
{
    if(!clip_provider_lock(provider)){
        return;
    }
    gtk_clipboard_set_text(provider->primary, "", -1);
    gtk_clipboard_set_text(provider->clipboard, "", -1);
    clip_provider_unlock(provider);
}






/**
 * Syncs the two provider clipboards and sets the provider's current value to match that of the synced value.
 */
static void clip_provider_sync_clipboards(ClipboardProvider* provider)
{
    if(!clip_provider_lock(provider)){
        return;
    }
	char* on_clipboard = gtk_clipboard_wait_for_text(provider->clipboard);
	char* on_primary = gtk_clipboard_wait_for_text(provider->primary);
    clip_provider_unlock(provider);

	char* selection = on_clipboard;

    // Check if the primary even has content.
	if(on_primary != NULL && strlen(on_primary) > 0){
        // Check if primary is different from clipboard. If it is, then make sure that it's the primary that changed and
        // not clipboard by comparing clipboard to "current".
        if(g_strcmp0(on_primary, on_clipboard) && !g_strcmp0(on_clipboard, provider->current)){
            // Primary definitely changed. Use it.
            selection = on_primary;
        }
	}
    clip_provider_set_current(provider, selection);
	g_free(on_clipboard);
	g_free(on_primary);
}

/**
 * Returns a copy of the current system clipboard.
 */
char* clip_provider_get_current(ClipboardProvider* provider)
{
    clip_provider_sync_clipboards(provider);

	return g_strdup(provider->current);
}

void clip_provider_free_current(char* current)
{
	g_free(current);
}
