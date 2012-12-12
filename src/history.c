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

#include "history.h"

#include "config.h"
#include "utils.h"

#include <stdlib.h>
#include <sqlite3.h>


#define HISTORY_CREATE_HISTORY "CREATE TABLE IF NOT EXISTS history(" \
                               "    id INTEGER PRIMARY KEY,"\
                               "    created TIMESTAMP NOT NULL DEFAULT current_timestamp,"\
                               "    text TEXT NOT NULL UNIQUE,"\
                               "    usage_count BIGINT NOT NULL DEFAULT 0,"\
                               "    locked TIMESTAMP"\
                               ")"
// The usage count will be 0 when null (i.e. no value).
#define HISTORY_INSERT_HISTORY "INSERT OR REPLACE INTO history(text, created, locked, usage_count) VALUES(?1, current_timestamp, "\
        "(SELECT locked FROM history WHERE text = ?1), "\
        "(SELECT usage_count + 1 FROM history WHERE text = ?1))"

#define HISTORY_DELETE_HISTORY "DELETE FROM history WHERE text = ? AND locked IS NULL"
#define HISTORY_TRUNCATE_HISTORY "DELETE FROM history WHERE locked IS NULL"
#define HISTORY_REMOVE_NEWEST_UNLOCKED "DELETE FROM history WHERE id = (SELECT max(id) FROM history WHERE locked IS NULL)"

// Use a LRU+LFU eviction policy.
#define HISTORY_EVICT_SINGLE "DELETE FROM history WHERE id = (SELECT id FROM history WHERE locked IS NULL ORDER BY usage_count, id LIMIT 1)"

#define HISTORY_SELECT_HISTORY "SELECT id, text, locked, usage_count FROM history ORDER BY created, id"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"

#define HISTORY_UPDATE_TOGGLE_LOCK "UPDATE history SET locked =   CASE WHEN locked IS NULL THEN current_timestamp     ELSE null END WHERE text = ?"


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

static void clip_history_storage_count(ClipboardHistory* history)
{
    int count_status = sqlite3_exec(history->storage, HISTORY_SELECT_COUNT, clip_history_callback_count, &history->count, NULL);
    if(SQLITE_OK != count_status){
        warn("Cannot attain history count (error %d).\n", count_status);
    }
}

static void clip_history_storage_open(ClipboardHistory* history)
{
    int connect_status = sqlite3_open(clip_config_get_storage_file(), &history->storage);
    if(SQLITE_OK != connect_status){
        warn("Cannot open persistent storage file, %s (error %d).\n", clip_config_get_storage_file(), connect_status);
    }

    int create_status = sqlite3_exec(history->storage, HISTORY_CREATE_HISTORY, NULL, NULL, NULL);
    if(SQLITE_OK != create_status){
        warn("Cannot create persistent storage schema (error %d).\n", create_status);
    }

    clip_history_storage_count(history);
}

ClipboardHistory* clip_history_new()
{
    ClipboardHistory* history = g_malloc(sizeof(ClipboardHistory));
    history->storage = NULL;
    history->count = 0;

    clip_history_storage_open(history);

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







/**
 * Returns the number of changed rows.
 */
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


static void clip_history_evict(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_EVICT_SINGLE, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot remove oldest history record (error %d).\n", status);
    }
    history->count -= sqlite3_changes(history->storage);
}


/**
 * Creates and returns a new history entry. Invokers are responsible for freeing it.
 */
void clip_history_prepend(ClipboardHistory* history, ClipboardEntry* entry)
{
    trace("Prepending/promoting text to history.\n");
    char* key = clip_clipboard_entry_get_text(entry);

    clip_history_storage_execute(HISTORY_INSERT_HISTORY, history, key);

    /*
     * Because this statement uses a create/replace, the number of affected rows
     * is not the same as inserted rows. This is basically a brute-force
     * approach that may need to change over time.
     */
    clip_history_storage_count(history);

    /* There are more entries than allowable. Remove the tail. */
    if(history->count > HISTORY_MAX_SIZE){
        clip_history_evict(history);
    }

    int id = sqlite3_last_insert_rowid(history->storage);
    debug("Created new history entry, %d.\n", id);
}

/**
 * Removes the entry from the history. This function does not free the entry, just removes it from the backing store.
 */
void clip_history_remove(ClipboardHistory* history, ClipboardEntry* entry)
{
    trace("Removing clipboard entry.\n");
    char* key = clip_clipboard_entry_get_text(entry);
    clip_history_storage_execute(HISTORY_DELETE_HISTORY, history, key);
}

/**
 * Removes the head entry from the history.
 */
void clip_history_remove_head(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_REMOVE_NEWEST_UNLOCKED, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot remove newest history record (error %d).\n", status);
    }
    history->count -= sqlite3_changes(history->storage);
}

void clip_history_toggle_lock(ClipboardHistory* history, ClipboardEntry* entry)
{
    trace("Toggling entry lock.\n");
    char* key = clip_clipboard_entry_get_text(entry);
    clip_history_storage_execute(HISTORY_UPDATE_TOGGLE_LOCK, history, key);
}



void clip_history_clear(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_TRUNCATE_HISTORY, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot truncate history table (error %d).\n", status);
    }
    history->count -= sqlite3_changes(history->storage);
}




static int clip_history_callback_selection(void* data, int row, char** values, char** names)
{
    // int id = atoi(values[0]);
    char* text = values[1];
    char* locked = values[2];
    unsigned int count = strtoul(values[3], NULL, 10);

    ClipboardEntry* entry = clip_clipboard_entry_new(text, locked != NULL, count);
    *((GList**)data) = g_list_prepend(*((GList**)data), entry);

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
    g_list_free_full(list, (GDestroyNotify)clip_clipboard_entry_free);
}
