#include <glib.h>

typedef struct history ClipboardHistory;

ClipboardHistory* clip_history_new(void);
void clip_history_free(ClipboardHistory* history);

void clip_history_prepend(ClipboardHistory* history, char* text);
void clip_history_remove(ClipboardHistory* history, char* text);
void clip_history_clear(ClipboardHistory* history);

GList* clip_history_get_list(ClipboardHistory* history);
void clip_history_free_list(GList* list);
