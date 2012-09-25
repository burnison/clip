#include "provider.h"

#include "utils.h"

#include <gtk/gtk.h>

struct provider {
	GtkClipboard* clipboard;
	GtkClipboard* primary;
};

ClipboardProvider* clip_provider_new()
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


static char* clip_provider_get_current_clipboard(ClipboardProvider* provider)
{
	return gtk_clipboard_wait_for_text(provider->clipboard);
}

static char* clip_provider_get_current_primary(ClipboardProvider* provider)
{
	return gtk_clipboard_wait_for_text(provider->primary);
}


char* clip_provider_get_current(ClipboardProvider* provider)
{
	char* on_clipboard = clip_provider_get_current_clipboard(provider);
	char* on_primary = clip_provider_get_current_primary(provider);
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
static void clip_provider_set_current_if_different(GtkClipboard* clipboard, char* old, char* new)
{
	if(g_strcmp0(old, new)){
		gtk_clipboard_set_text(clipboard, new, -1);
	}
}

void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
	char* on_clipboard = clip_provider_get_current_clipboard(provider);
	char* on_primary = clip_provider_get_current_primary(provider);

	clip_provider_set_current_if_different(provider->clipboard, on_clipboard, text);
	clip_provider_set_current_if_different(provider->primary, on_primary, text);

	g_free(on_clipboard);
	g_free(on_primary);
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
