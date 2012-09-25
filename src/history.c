#include "history.h"

#include "config.h"
#include "utils.h"


struct history {;
    GList* list;
};


ClipboardHistory* clip_history_new()
{
    ClipboardHistory* history = g_malloc(sizeof(ClipboardHistory));
    history->list = NULL;
    return history;
}

void clip_history_free(ClipboardHistory* history)
{
    if(history == NULL){
        warn("Attempted to free NULL history.\n");
        return;
    }

    g_list_free_full(history->list, g_free);
    history->list = NULL;

    g_free(history);
}



static void clip_history_remove_entry(ClipboardHistory* history, GList* entry)
{
    char* data = entry->data;
    history->list = g_list_remove(history->list, data);
    g_free(data);
}

static void clip_history_remove_if_exists(ClipboardHistory* history, char* text)
{
    GList* entry = g_list_first(history->list);
    while((entry = g_list_next(entry))){
        if(!g_strcmp0(entry->data, text)){
            clip_history_remove_entry(history, entry);
            break;
        }
    }
}

void clip_history_prepend(ClipboardHistory* history, char* text)
{
    // Remove this value if it exists in the history.
    // This method also perfroms the free.
    clip_history_remove_if_exists(history, text);

    // Put the new value at the head of the list.
    char* data = g_strdup(text);
    history->list = g_list_prepend(history->list, data);

    // There are more entries than allowable. Remove the tail.
    if(HISTORY_MAX_SIZE <= g_list_length(history->list)){
        GList* last = g_list_last(history->list);
        clip_history_remove_entry(history, last);
    }
}

void clip_history_remove(ClipboardHistory* history, char* text)
{
    GList* entry = g_list_find(history->list, text);
    if(entry == NULL){
        warn("Tried to remove a value that isn't in clipboard history.");
        return;
    }
    clip_history_remove_entry(history, entry);
}

void clip_history_clear(ClipboardHistory* history)
{
    g_list_free_full(history->list, (GDestroyNotify)g_free);
    history->list = NULL;
}



GList* clip_history_get_list(ClipboardHistory* history)
{
    return g_list_copy(history->list);
}

void clip_history_free_list(GList* list)
{
    g_list_free(list);
}
