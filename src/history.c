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
#define HISTORY_INSERT_NEW_HISTORY "INSERT OR REPLACE INTO history(id, text, created, locked, usage_count) VALUES( "\
                               "(SELECT id FROM history WHERE text = ?1), "\
                               "?1, current_timestamp, "\
                               "(SELECT locked FROM history WHERE text = ?1), "\
                               "(SELECT usage_count + 1 FROM history WHERE text = ?1))"
#define HISTORY_INSERT_EXISTING_HISTORY "UPDATE history SET text = ?1, created = current_timestamp WHERE id = ?2"
#define HISTORY_UPDATE_HISTORY "UPDATE history SET text = ?1 WHERE id = ?2"

#define HISTORY_DELETE_HISTORY "DELETE FROM history WHERE id = ? AND locked IS NULL"
#define HISTORY_TRUNCATE_HISTORY "DELETE FROM history WHERE locked IS NULL"
#define HISTORY_REMOVE_NEWEST_UNLOCKED "DELETE FROM history WHERE id = (SELECT max(id) FROM history WHERE locked IS NULL)"
#define HISTORY_REPLACE_HISTORY "UPDATE history SET text = ?1 null END WHERE text = ?2"

// Use a LRU+LFU eviction policy.
#define HISTORY_EVICT_SINGLE "DELETE FROM history WHERE id = (SELECT id FROM history WHERE locked IS NULL ORDER BY usage_count, id LIMIT 1)"

#define HISTORY_SELECT_HISTORY "SELECT id, text, locked, usage_count FROM history ORDER BY created, id"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"

#define HISTORY_UPDATE_TOGGLE_LOCK "UPDATE history SET locked =   CASE WHEN locked IS NULL THEN current_timestamp     ELSE null END WHERE id = ?"


struct history {;
    sqlite3* storage;
    int count;
};

static void clip_history_storage_count(ClipboardHistory* history)
{
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_SELECT_COUNT, -1, &statement, NULL);
    if(status != SQLITE_OK){
        warn("Cannot attain history count (error %d).\n", status);
    } else if(sqlite3_step(statement) == SQLITE_ROW){
        history->count = sqlite3_column_int64(statement, 0);
        debug("History currently has %d records.\n", history->count);
    }
    sqlite3_finalize(statement);
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


static void clip_history_evict(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_EVICT_SINGLE, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot remove oldest history record (error %d).\n", status);
        return;
    }
    history->count -= sqlite3_changes(history->storage);
}



static void clip_history_prepend_new(ClipboardHistory* history, ClipboardEntry* entry)
{
    trace("Prepending new entry.\n");

    sqlite3_stmt* statement = NULL;
    char* text = clip_clipboard_entry_get_text(entry);

    int status = sqlite3_prepare(history->storage, HISTORY_INSERT_NEW_HISTORY, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        if((status = sqlite3_step(statement)) == SQLITE_DONE){
            int64_t id = sqlite3_last_insert_rowid(history->storage);
            clip_clipboard_entry_set_id(entry, id);
            debug("Created new history entry, %"PRIu64".\n", id);
        } else {
            warn("Couldn't prepend new entry (error %d).\n", status);
        }
    } else {
        warn("Couldn't prepare entry prepend (error %d).\n", status);
    }
    sqlite3_finalize(statement);
}

static void clip_history_prepend_existing(ClipboardHistory* history, ClipboardEntry* entry)
{
    sqlite3_stmt* statement = NULL;
    int64_t id = clip_clipboard_entry_get_id(entry);
    char* text = clip_clipboard_entry_get_text(entry);

    trace("Promoting existing entry, %"PRIu64", to top.\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_INSERT_EXISTING_HISTORY, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 2, id);
        if((status = sqlite3_step(statement)) != SQLITE_DONE){
            warn("Couldn't prepend existing entry, %"PRIu64" (error %d).\n", id, status);
        }
    } else {
        warn("Couldn't prepare entry prepend for %"PRIu64" (error %d).\n", id, status);
    }
    sqlite3_finalize(statement);
}

/**
 * Creates and returns a new history entry. Invokers are responsible for freeing it.
 */
void clip_history_prepend(ClipboardHistory* history, ClipboardEntry* entry)
{
    if(clip_clipboard_entry_is_new(entry)){
        clip_history_prepend_new(history, entry);
    } else {
        clip_history_prepend_existing(history, entry);
    }

    // Reacquire the new count (an insert may have been a replace).
    clip_history_storage_count(history);
    if(history->count > HISTORY_MAX_SIZE){
        clip_history_evict(history);
    }
}



void clip_history_update(ClipboardHistory* history, ClipboardEntry* entry)
{
    sqlite3_stmt* statement = NULL;
    int64_t id = clip_clipboard_entry_get_id(entry);
    char* text = clip_clipboard_entry_get_text(entry);

    trace("Updating existing entry, %"PRIu64".\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_UPDATE_HISTORY, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 2, id);
        if((status = sqlite3_step(statement)) != SQLITE_DONE){
            warn("Couldn't update entry, %"PRIu64" (error %d).\n", id, status);
        }
    } else {
        warn("Couldn't prepare entry update for %"PRIu64" (error %d).\n", id, status);
    }
    sqlite3_finalize(statement);
}




/**
 * Removes the entry from the history. This function does not free the entry, just removes it from the backing store.
 */
void clip_history_remove(ClipboardHistory* history, ClipboardEntry* entry)
{
    int64_t id = clip_clipboard_entry_get_id(entry);
    trace("Removing clipboard entry %"PRIu64".\n", id);

    sqlite3_stmt* statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_DELETE_HISTORY, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_int64(statement, 1, id);
        if(sqlite3_step(statement) != SQLITE_DONE){
            warn("Couldn't remove entry, %"PRIu64" (error %d).\n", id, status);
        }
    } else {
        warn("Couldn't prepare entry removal query for %"PRIu64" (error %d).\n", id, status);
    }
    sqlite3_finalize(statement);
}


ClipboardEntry* clip_history_get_head(ClipboardHistory* history)
{
    ClipboardEntry *head = NULL;
    GList *list = clip_history_get_list(history);
    if(list == NULL){
        goto exit;
    }

    head = clip_clipboard_entry_clone(g_list_first(list)->data);

exit:
    clip_history_free_list(list);
    return head;
}

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
    int64_t id = clip_clipboard_entry_get_id(entry);
    trace("Toggling entry lock for %"PRIu64".\n", id);

    sqlite3_stmt* statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_UPDATE_TOGGLE_LOCK, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_int64(statement, 1, id);
        if(sqlite3_step(statement) != SQLITE_DONE){
            warn("Couldn't toggle lock for %"PRIu64" (error %d).\n", id, status);
        }
    } else {
        warn("Couldn't prepare lock toggle query for %"PRIu64" (error %d).\n", id, status);
    }
    sqlite3_finalize(statement);
}



void clip_history_clear(ClipboardHistory* history)
{
    int status = sqlite3_exec(history->storage, HISTORY_TRUNCATE_HISTORY, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot truncate history table (error %d).\n", status);
    }
    history->count -= sqlite3_changes(history->storage);
}


GList* clip_history_get_list(ClipboardHistory* history)
{
    GList* list = NULL;
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_SELECT_HISTORY, -1, &statement, NULL);
    if(status != SQLITE_OK){
        warn("Cannot prepare history selection query (error %d).\n", status);
    } else {
        while(sqlite3_step(statement) == SQLITE_ROW){
            int64_t id = sqlite3_column_int64(statement, 0);
            char* text = (char*)sqlite3_column_text(statement, 1);
            char* locked = (char*)sqlite3_column_text(statement, 2);
            int count = sqlite3_column_int(statement, 3);
            ClipboardEntry* entry = clip_clipboard_entry_new(id, text, locked != NULL, count);
            list = g_list_prepend(list, entry);
        }
    }
    sqlite3_finalize(statement);
    return list;
}

void clip_history_free_list(GList* list)
{
    g_list_free_full(list, (GDestroyNotify)clip_clipboard_entry_free);
}
