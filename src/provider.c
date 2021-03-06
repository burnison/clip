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

#include "provider.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <string.h>

struct provider {
    GtkClipboard *clipboard;
#if SYNC_ANY
    GtkClipboard *selection;
#endif
    char *current;
    gboolean ownership_transferred;
    gboolean locked;
};


static void clip_provider_unlock(ClipboardProvider *provider)
{
    provider->locked = FALSE;
}


void clip_provider_cb_owner_changed(GtkClipboard *clipboard, GdkEvent *event, gpointer data)
{
    if(((GdkEventOwnerChange*)event)->reason != GDK_OWNER_CHANGE_NEW_OWNER) {
        debug("Clipboard owner has been destroyed. Going to ignore next value if null.\n");
        ((ClipboardProvider*)data)->ownership_transferred = TRUE;
    }
}

ClipboardProvider* clip_provider_new(void)
{
    ClipboardProvider *provider = g_malloc(sizeof(ClipboardProvider));
    provider->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
#if SYNC_ANY
    provider->selection = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
#endif
    provider->current = NULL;
    provider->ownership_transferred = FALSE;
    provider->locked = FALSE;

    g_signal_connect(G_OBJECT(provider->clipboard), "owner-change", G_CALLBACK(clip_provider_cb_owner_changed), provider);
#if SYNC_CLIPBOARDS
    g_signal_connect(G_OBJECT(provider->selection), "owner-change", G_CALLBACK(clip_provider_cb_owner_changed), provider);
#endif

    return provider;
}

void clip_provider_free(ClipboardProvider *provider)
{
    if(provider == NULL){
        return;
    }

    g_signal_handlers_disconnect_by_data(provider->clipboard, provider);
    provider->clipboard = NULL;

#if SYNC_CLIPBOARDS
    g_signal_handlers_disconnect_by_data(provider->selection, provider);
#endif
#if SYNC_ANY
    provider->selection = NULL;
#endif

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
static gboolean clip_provider_lock(ClipboardProvider *provider)
{
    if(provider->locked){
        return FALSE;
    }
    provider->locked = TRUE;
    return TRUE;
}

/**
 * Determines if the clipboards are in a state wherein content can be used. For example, if the right mouse button is
 * currently pressed, we can assume that the selection clipboard is not fully complete, and thus, the content is not yet
 * ready to be consumed.
 */
gboolean clip_provider_is_provider_ready()
{
    GdkWindow *root_window = gdk_get_default_root_window();
    GdkDeviceManager *device_manager = gdk_display_get_device_manager(gdk_display_get_default());
    GdkDevice *pointer = gdk_device_manager_get_client_pointer(device_manager);
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
static gboolean clip_provider_set_if_different(GtkClipboard *clipboard, char *new)
{
    gboolean changed = FALSE;
    char *old = gtk_clipboard_wait_for_text(clipboard);
    if(g_strcmp0(old, new)){
        gtk_clipboard_set_text(clipboard, new == NULL ? "" : new, -1);
        changed = TRUE;
    }
    g_free(old);
    return changed;
}

static char* clip_provider_prepare_value(ClipboardProvider *provider, char *text)
{
    if(provider->ownership_transferred){
        provider->ownership_transferred = FALSE;
        if(text == NULL){
            debug("Encountered a null after ownership transfer. Dropping and reverting current head.\n");
            return g_strdup(provider->current);
        }
    }
    return g_strdup(text);
}

/**
 * Sets the provider clipboards to a copy of the specified value.
 */
void clip_provider_set_current(ClipboardProvider *provider, char *text)
{
    char *copy = clip_provider_prepare_value(provider, text);
    if(!clip_provider_lock(provider)){
        g_free(copy);
        return;
    }

    gboolean clipboard_changed = clip_provider_set_if_different(provider->clipboard, copy);
// Things get a bit wonky here: if we're syncing clipboards, then primary is always up-to-date (that is, a change to
// primary becomes the authoritative change). However, when only syncing *to* primary, a change to primary is not the
// authoritative change, thus, only copy to primary iff clipboard changes.
#if SYNC_CLIPBOARDS
    clip_provider_set_if_different(provider->selection, copy);
#elif SYNC_PRIMARY
    if(clipboard_changed) {
        clip_provider_set_if_different(provider->selection, copy);
    }
#endif
    clip_provider_unlock(provider);

    char *old = provider->current;
    provider->current = copy;
    g_free(old);
}

void clip_provider_clear(ClipboardProvider *provider)
{
    if(!clip_provider_lock(provider)){
        return;
    }
    gtk_clipboard_set_text(provider->clipboard, "", -1);
#if SYNC_ANY
    gtk_clipboard_set_text(provider->selection, "", -1);
#endif
    clip_provider_unlock(provider);
}






/**
 * Syncs the two provider clipboards and sets the provider's current value to match that of the synced value.
 */
static void clip_provider_sync_clipboards(ClipboardProvider *provider)
{
    if(!clip_provider_lock(provider)){
        return;
    }
    char *on_clipboard = gtk_clipboard_wait_for_text(provider->clipboard);
    char *selection = on_clipboard;
#if SYNC_CLIPBOARDS
    char *on_primary = gtk_clipboard_wait_for_text(provider->selection);
    // Check if the selection even has content.
    if(on_primary != NULL && strlen(on_primary) > 0){
        // Check if selection is different from clipboard. If it is, then make sure that it's the selection that changed and
        // not clipboard by comparing clipboard to "current".
        if(g_strcmp0(on_primary, on_clipboard) && !g_strcmp0(on_clipboard, provider->current)){
            // selection definitely changed. Use it.
            selection = on_primary;
        }
    }
#endif
    clip_provider_unlock(provider);
    clip_provider_set_current(provider, selection);
    g_free(on_clipboard);
#if SYNC_CLIPBOARDS
    g_free(on_primary);
#endif
}

/**
 * Returns a copy of the current system clipboard.
 */
char* clip_provider_get_current(ClipboardProvider *provider)
{
    clip_provider_sync_clipboards(provider);
    return g_strdup(provider->current);
}

void clip_provider_free_current(char *current)
{
    g_free(current);
}
