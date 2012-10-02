#include "history.h"

#include "config.h"
#include "utils.h"

#include <stdlib.h>
#include <sqlite3.h>


#define HISTORY_CREATE_HISTORY "CREATE TABLE IF NOT EXISTS history(" \
                               "    id INTEGER PRIMARY KEY,"\
                               "    created TIMESTAMP NOT NULL DEFAULT current_timestamp,"\
                               "    text TEXT NOT NULL UNIQUE"\
                               ")"
#define HISTORY_INSERT_HISTORY "INSERT INTO history(text) VALUES(?)"
#define HISTORY_DELETE_HISTORY "DELETE FROM history WHERE text = ?"
#define HISTORY_TRUNCATE_HISTORY "DELETE FROM history"
#define HISTORY_REMOVE_OLDEST "DELETE FROM history WHERE id = (SELECT min(id) FROM history)"
#define HISTORY_SELECT_HISTORY "SELECT text FROM history ORDER BY created, id"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"


struct history {;
    sqlite3* storage;
    int count;
};


static int clip_history_callback_count(void* data, int row, char** values, char** names)
{
    *((int*)data) = atoi(values[0]);
    debug("History currently has %d records.\n", *(int*)data);
    return 0;
}

static void clip_history_open_storage(ClipboardHistory* history)
{
    int connect_status = sqlite3_open(clip_config_get_storage_file(), &history->storage);
    if(SQLITE_OK != connect_status){
        warn("Cannot open persistent storage file, %s (error %d).\n", clip_config_get_storage_file(), connect_status);
    }

    int create_status = sqlite3_exec(history->storage, HISTORY_CREATE_HISTORY, NULL, NULL, NULL);
    if(SQLITE_OK != create_status){
        warn("Cannot create persistent storage schema (error %d).\n", create_status);
    }

    int count_status = sqlite3_exec(history->storage, HISTORY_SELECT_COUNT, clip_history_callback_count, &history->count, NULL);
    if(SQLITE_OK != count_status){
        warn("Cannot attain history count (error %d).\n", count_status);
    }
}

ClipboardHistory* clip_history_new()
{
    ClipboardHistory* history = g_malloc(sizeof(ClipboardHistory));
    history->storage = NULL;
    history->count = 0;

    clip_history_open_storage(history);

    return history;
}


static void clip_history_close_storage(ClipboardHistory* history)
{
    // It's possible the storage didn't get opened.
    if(history->storage != NULL){
        sqlite3_close(history->storage);
        history->storage = NULL;
    }
}

void clip_history_free(ClipboardHistory* history)
{
    if(history == NULL){
        warn("Attempted to free NULL history.\n");
        return;
    }

    clip_history_close_storage(history);

    g_free(history);
}







static int clip_history_storage_execute(const char* query, ClipboardHistory* history, char* text)
{
    int status = 0;
    sqlite3_stmt* statement = NULL;

    status = sqlite3_prepare(history->storage, query, -1, &statement, NULL);
    if(SQLITE_OK != status || statement == NULL){
        warn("Couldn't prepare history statement (error %d).\n", status);
        goto finalize;
    }

    status = sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
    if(SQLITE_OK != status){
        warn("Couldn't bind text to history statement (error %d).\n", status);
        goto finalize;
    }

    status = sqlite3_step(statement);
    if(SQLITE_DONE != status){
        warn("Couldn't execute history statement (error %d).\n", status);
        goto finalize;
    }

finalize:
    if(statement != NULL){
        sqlite3_finalize(statement);
    }
    return sqlite3_changes(history->storage);
}



void clip_history_prepend(ClipboardHistory* history, char* text)
{
    // Put the new value at the head of the list. Inefficient, but meh for now.
    history->count -= clip_history_storage_execute(HISTORY_DELETE_HISTORY, history, text);
    history->count += clip_history_storage_execute(HISTORY_INSERT_HISTORY, history, text);

    // There are more entries than allowable. Remove the tail.
    if(history->count > HISTORY_MAX_SIZE){
        int status = sqlite3_exec(history->storage, HISTORY_REMOVE_OLDEST, NULL, NULL, NULL);
        if(SQLITE_OK != status){
            warn("Cannot remove oldest history record (error %d).\n", status);
        }
        history->count -= sqlite3_changes(history->storage);
    }

    trace("History has %d entries.\n", history->count);
}

void clip_history_remove(ClipboardHistory* history, char* text)
{
    clip_history_storage_execute(HISTORY_DELETE_HISTORY, history, text);
}

void clip_history_clear(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_TRUNCATE_HISTORY, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot truncate history table (error %d).\n", status);
    }
}


static int clip_history_callback_selection(void* data, int row, char** values, char** names)
{
    *((GList**)data) = g_list_prepend(*((GList**)data), g_strdup(values[0]));

    return 0;
}

GList* clip_history_get_list(ClipboardHistory* history)
{
    int dummy = 1;
    int* pdummy = &dummy;
    GList* list = g_list_prepend((GList*)NULL, pdummy);

    int status = sqlite3_exec(history->storage, HISTORY_SELECT_HISTORY, clip_history_callback_selection, &list, NULL);
    if(SQLITE_OK != status){
        error("Couldn't return history list (error %d).\n", status);
        return NULL;
    }
    list = g_list_remove(list, pdummy);

    return list;
}

void clip_history_free_list(GList* list)
{
    // The callback calls g_strdup.
    g_list_free_full(list, g_free);
}
