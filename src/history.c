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
                               "    locked TIMESTAMP"\
                               ")"
#define HISTORY_INSERT_HISTORY "INSERT OR REPLACE INTO history(text, created, locked) VALUES(?1, current_timestamp, (SELECT locked FROM history WHERE text = ?1))"

#define HISTORY_DELETE_HISTORY "DELETE FROM history WHERE text = ? AND locked IS NULL"
#define HISTORY_TRUNCATE_HISTORY "DELETE FROM history WHERE locked IS NULL"
#define HISTORY_REMOVE_OLDEST_UNLOCKED "DELETE FROM history WHERE id = (SELECT min(id) FROM history WHERE locked IS NULL)"
#define HISTORY_REMOVE_NEWEST_UNLOCKED "DELETE FROM history WHERE id = (SELECT max(id) FROM history WHERE locked IS NULL)"

#define HISTORY_SELECT_HISTORY "SELECT id, text, locked FROM history ORDER BY created, id"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"

#define HISTORY_UPDATE_TOGGLE_LOCK "UPDATE history SET locked =   CASE WHEN locked IS NULL THEN current_timestamp     ELSE null END WHERE text = ?"


struct history {;
    sqlite3* storage;
    int count;
};

struct entry {
    int id;
    char* text;
    gboolean locked;
};

/**
 * Creates a new history entry using copies of the provided texts.
 */
static ClipboardHistoryEntry* clip_history_entry_new(int id, char* text, char* locked)
{
    ClipboardHistoryEntry* entry = g_malloc(sizeof(ClipboardHistoryEntry));
    entry->id = id;
    entry->text = g_strdup(text);
    entry->locked = (locked == NULL) ? FALSE : TRUE;
    return entry;
}

static void clip_history_entry_free(ClipboardHistoryEntry* entry)
{
    g_free(entry->text);
    entry->text = NULL;

    g_free(entry);
}




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




char* clip_history_entry_get_text(ClipboardHistoryEntry* entry)
{
    return g_strdup(entry->text);
}

void clip_history_entry_free_text(char* text)
{
    g_free(text);
}


gboolean clip_history_entry_get_locked(ClipboardHistoryEntry* entry)
{
    return entry->locked;
}



void clip_history_entry_toggle_lock(ClipboardHistory* history, ClipboardHistoryEntry* entry)
{
    trace("Toggling entry lock.\n");
    clip_history_storage_execute(HISTORY_UPDATE_TOGGLE_LOCK, history, entry->text);
    entry->locked = !entry->locked;
}


/**
 * Creates and returns a new history entry. Invokers are responsible for freeing it.
 */
void clip_history_prepend(ClipboardHistory* history, char* text)
{
    trace("Prepending/promoting text to history.\n");
    clip_history_storage_execute(HISTORY_INSERT_HISTORY, history, text);

    /*
     * Because this statement uses a create/replace, the number of affected rows
     * is not the same as inserted rows. This is basically a brute-force
     * approach that may need to change over time.
     */
    clip_history_storage_count(history);

    /* There are more entries than allowable. Remove the tail. */
    if(history->count > HISTORY_MAX_SIZE){
        int status = sqlite3_exec(history->storage, HISTORY_REMOVE_OLDEST_UNLOCKED, NULL, NULL, NULL);
        if(SQLITE_OK != status){
            warn("Cannot remove oldest history record (error %d).\n", status);
        }
        history->count -= sqlite3_changes(history->storage);
    }

    int id = sqlite3_last_insert_rowid(history->storage);
    debug("Created new history entry, %d.\n", id);
}

/**
 * Removes the entry from the history. This function does not free the entry, just removes it from the backing store.
 */
void clip_history_remove(ClipboardHistory* history, ClipboardHistoryEntry* entry)
{
    debug("Removing clipboard entry, %d.\n", entry->id);
    clip_history_storage_execute(HISTORY_DELETE_HISTORY, history, entry->text);
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
    ClipboardHistoryEntry* entry = clip_history_entry_new(atoi(values[0]), values[1], values[2]);

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
    g_list_free_full(list, (GDestroyNotify)clip_history_entry_free);
}
