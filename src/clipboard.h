#include "provider.h"

#include <glib.h>

typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(ClipboardProvider* provider);
void clip_clipboard_free(Clipboard* clipboard);

char* clip_clipboard_get_active(Clipboard* clipboard);
void clip_clipboard_set_active(Clipboard* clipboard, char* text);

GList* clip_clipboard_get_history(Clipboard* clipboard);
void clip_clipboard_free_history(GList* history);
