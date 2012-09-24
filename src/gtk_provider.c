#include "provider.h"

#include "utils.h"

#include <gtk/gtk.h>

struct provider {
	GtkClipboard* clipboard;
	GtkClipboard* primary;
	char* head;
};

ClipboardProvider* clip_provider_new()
{
	ClipboardProvider* provider = g_malloc(sizeof(ClipboardProvider));
	provider->clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	provider->primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	provider->head = NULL;

	return provider;
}

void clip_provider_free(ClipboardProvider* provider)
{
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
 * @param head The new value to set.
 * @param current The current clipboard contents.
 */
static void clip_provider_set_current_if_different(GtkClipboard* clipboard, char* head, char* current)
{
	if(g_strcmp0(head, current)){
		gtk_clipboard_set_text(clipboard, head, -1);
	}
}

void clip_provider_set_current(ClipboardProvider* provider, char* text)
{
	char* on_clipboard = clip_provider_get_current_clipboard(provider);
	char* on_primary = clip_provider_get_current_primary(provider);
	char* old_head = provider->head;

	provider->head = g_strdup(text);
	clip_provider_set_current_if_different(provider->clipboard, provider->head, on_clipboard);
	clip_provider_set_current_if_different(provider->primary, provider->head, on_primary);

	g_free(old_head);
	g_free(on_clipboard);
	g_free(on_primary);
}
