#include "provider.h"

#include <glib.h>

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider* provider);
void clip_clipboard_free(Clipboard* clipboard);

// Enables/disables this clipboard's history. Enabling will force a flush.
void clip_clipboard_toggle(Clipboard* clipboard);
gboolean clip_clipboard_is_enabled(Clipboard* clipboard);

void clip_clipboard_clear(Clipboard* clipboard);

void clip_clipboard_set_active(Clipboard* clipboard, char* text);
char* clip_clipboard_get_active(Clipboard* clipboard);
void clip_clipboard_free_active(char* text);

GList* clip_clipboard_get_history(Clipboard* clipboard);
void clip_clipboard_free_history(GList* history);
