#include <glib.h>

typedef struct history ClipboardHistory;
typedef struct entry ClipboardHistoryEntry;


ClipboardHistory* clip_history_new(void);
void clip_history_free(ClipboardHistory* history);


char* clip_history_entry_get_text(ClipboardHistoryEntry* entry);
void clip_history_entry_free_text(char* text);

gboolean clip_history_entry_get_locked(ClipboardHistoryEntry* entry);
void clip_history_entry_toggle_lock(ClipboardHistory* history, ClipboardHistoryEntry* entry);

void clip_history_prepend(ClipboardHistory* history, char* text);
void clip_history_remove(ClipboardHistory* history, ClipboardHistoryEntry* entry);

void clip_history_clear(ClipboardHistory* history);

GList* clip_history_get_list(ClipboardHistory* history);
void clip_history_free_list(GList* list);
