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
#include <string.h>
#include <sqlite3.h>


#define HISTORY_CREATE "CREATE TABLE IF NOT EXISTS history(" \
                               "    id INTEGER PRIMARY KEY,"\
                               "    created TIMESTAMP NOT NULL DEFAULT current_timestamp,"\
                               "    text TEXT NOT NULL UNIQUE,"\
                               "    usage_count BIGINT NOT NULL DEFAULT 0,"\
                               "    locked TIMESTAMP"\
                               ")"

#define HISTORY_INSERT_EXISTING "UPDATE history SET text = ?1, created = current_timestamp WHERE id = ?2"
#define HISTORY_INSERT_NEW "INSERT OR REPLACE INTO history(id, text, created, locked, usage_count) VALUES( "\
                               "(SELECT id FROM history WHERE text = ?1), "\
                               "?1, current_timestamp, "\
                               "(SELECT locked FROM history WHERE text = ?1), "\
                               "(SELECT usage_count + 1 FROM history WHERE text = ?1))"

#define HISTORY_UPDATE_BY_ID_TEXT "UPDATE history SET text = ?1 WHERE id = ?2"
#define HISTORY_UPDATE_BY_ID_TOGGLE_LOCK "UPDATE history SET locked =   CASE WHEN locked IS NULL THEN current_timestamp     ELSE null END WHERE id = ?"

#define HISTORY_DELETE_UNLOCKED_BY_ID "DELETE FROM history WHERE id = ? AND locked IS NULL"
#define HISTORY_DELETE_UNLOCKED_BY_AGE "DELETE FROM history WHERE id = (SELECT max(id) FROM history WHERE locked IS NULL)"

#define HISTORY_CLEAR "DELETE FROM history WHERE locked IS NULL"

// Use a LRU+LFU eviction policy.
#define HISTORY_EVICT_SINGLE "DELETE FROM history WHERE id = (SELECT id FROM history WHERE locked IS NULL ORDER BY usage_count, id LIMIT 1)"

#define HISTORY_SELECT_ALL "SELECT id, text, locked, usage_count FROM history ORDER BY created, id"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"
#define HISTORY_SELECT_BY_TEXT "SELECT id, text, locked, usage_count FROM history WHERE text = ?1"

static int levenshtein_distance(const char *s, const char *t);
static ClipboardEntry* clip_history_get_by_text(ClipboardHistory *history, char *text);

struct history {;
    sqlite3 *storage;
    int count;
};

static void clip_history_storage_count(ClipboardHistory *history)
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

static void clip_history_storage_open(ClipboardHistory *history)
{
    int connect_status = sqlite3_open(clip_config_get_storage_file(), &history->storage);
    if(SQLITE_OK != connect_status){
        warn("Cannot open persistent storage file, %s (error %d).\n", clip_config_get_storage_file(), connect_status);
    }

    int create_status = sqlite3_exec(history->storage, HISTORY_CREATE, NULL, NULL, NULL);
    if(SQLITE_OK != create_status){
        warn("Cannot create persistent storage schema (error %d).\n", create_status);
    }

    clip_history_storage_count(history);
}

ClipboardHistory* clip_history_new()
{
    ClipboardHistory *history = g_malloc(sizeof(ClipboardHistory));
    history->storage = NULL;
    history->count = 0;

    clip_history_storage_open(history);

    return history;
}

void clip_history_free(ClipboardHistory *history)
{
    if(history == NULL){
        warn("Attempted to free NULL history.\n");
        return;
    } else if(history->storage != NULL){
        sqlite3_close(history->storage);
        history->storage = NULL;
    }
    g_free(history);
}

void clip_history_free_list(GList *list)
{
    g_list_free_full(list, (GDestroyNotify)clip_clipboard_entry_free);
}


static void clip_history_evict(ClipboardHistory *history)
{
    int status = sqlite3_exec(history->storage, HISTORY_EVICT_SINGLE, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot remove oldest history record (error %d).\n", status);
        return;
    }
    history->count -= sqlite3_changes(history->storage);
}



static gboolean clip_history_prepend_new(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success = TRUE;
    sqlite3_stmt *statement = NULL;
    char *text = clip_clipboard_entry_get_text(entry);

    trace("Prepending new entry.\n");
    int status = sqlite3_prepare(history->storage, HISTORY_INSERT_NEW, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        if((status = sqlite3_step(statement)) == SQLITE_DONE){
            int64_t id = sqlite3_last_insert_rowid(history->storage);
            clip_clipboard_entry_set_id(entry, id);
            debug("Created new history entry, %"PRIu64".\n", id);
        } else {
            warn("Couldn't prepend new entry (error %d).\n", status);
            success = FALSE;
        }
    } else {
        warn("Couldn't prepare entry prepend (error %d).\n", status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
    return success;
}

static gboolean clip_history_prepend_existing(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success = TRUE;
    sqlite3_stmt *statement = NULL;
    int64_t id = clip_clipboard_entry_get_id(entry);
    char *text = clip_clipboard_entry_get_text(entry);

    trace("Promoting existing entry, %"PRIu64", to top.\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_INSERT_EXISTING, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 2, id);
        if((status = sqlite3_step(statement)) != SQLITE_DONE){
            warn("Couldn't prepend existing entry, %"PRIu64" (error %d).\n", id, status);
            success = FALSE;
        }
    } else {
        warn("Couldn't prepare entry prepend for %"PRIu64" (error %d).\n", id, status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
    return success;
}

/**
 * Creates and returns a new history entry. Invokers are responsible for freeing it.
 */
gboolean clip_history_prepend(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success;
    if(clip_clipboard_entry_is_new(entry)){
        success = clip_history_prepend_new(history, entry);
    } else {
        success = clip_history_prepend_existing(history, entry);
    }

    // Reacquire the new count (an insert may have been a replace).
    clip_history_storage_count(history);
    if(history->count > HISTORY_MAX_SIZE){
        clip_history_evict(history);
    }
    return success;
}

gboolean clip_history_update(ClipboardHistory *history, ClipboardEntry *entry)
{
    int64_t id = clip_clipboard_entry_get_id(entry);
    char *text = clip_clipboard_entry_get_text(entry);


    ClipboardEntry *existing = clip_history_get_by_text(history, text);
    if(existing != NULL){
        debug("Entry, %"PRIu64", already has that value.\n", id);
        gboolean deleted = clip_history_remove(history, existing);
        clip_clipboard_entry_free(existing);
        if(!deleted){
            warn("Couldn't remove existing record, %"PRIu64" with desired text.\n", id);
            return FALSE;
        }
    }


    gboolean success = TRUE;
    sqlite3_stmt *statement = NULL;
    trace("Updating existing entry, %"PRIu64".\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_UPDATE_BY_ID_TEXT, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 2, id);
        if((status = sqlite3_step(statement)) != SQLITE_DONE){
            warn("Couldn't update entry, %"PRIu64" (error %d).\n", id, status);
            success = FALSE;
        }
    } else {
        warn("Couldn't prepare entry update for %"PRIu64" (error %d).\n", id, status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
    return success;
}


gboolean clip_history_toggle_lock(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success = TRUE;
    sqlite3_stmt *statement = NULL;
    int64_t id = clip_clipboard_entry_get_id(entry);

    trace("Toggling entry lock for %"PRIu64".\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_UPDATE_BY_ID_TOGGLE_LOCK, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_int64(statement, 1, id);
        if(sqlite3_step(statement) != SQLITE_DONE){
            warn("Couldn't toggle lock for %"PRIu64" (error %d).\n", id, status);
            success = FALSE;
        }
    } else {
        warn("Couldn't prepare lock toggle query for %"PRIu64" (error %d).\n", id, status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
    return success;
}


/**
 * Removes the entry from the history. This function does not free the entry, just removes it from the backing store.
 */
gboolean clip_history_remove(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success = TRUE;
    sqlite3_stmt *statement = NULL;
    int64_t id = clip_clipboard_entry_get_id(entry);

    trace("Removing clipboard entry %"PRIu64".\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_DELETE_UNLOCKED_BY_ID, -1, &statement, NULL);
    if(status == SQLITE_OK){
        sqlite3_bind_int64(statement, 1, id);
        if(sqlite3_step(statement) != SQLITE_DONE){
            warn("Couldn't remove entry, %"PRIu64" (error %d).\n", id, status);
            success = FALSE;
        }
    } else {
        warn("Couldn't prepare entry removal query for %"PRIu64" (error %d).\n", id, status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
    return success;
}

gboolean clip_history_remove_head(ClipboardHistory *history)
{
    gboolean success = TRUE;
    int status = sqlite3_exec(history->storage, HISTORY_DELETE_UNLOCKED_BY_AGE, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot remove newest history record (error %d).\n", status);
        success = FALSE;
    }
    history->count -= sqlite3_changes(history->storage);
    return success;
}

void clip_history_clear(ClipboardHistory *history)
{
    int status = sqlite3_exec(history->storage, HISTORY_CLEAR, NULL, NULL, NULL);
    if(SQLITE_OK != status){
        warn("Cannot truncate history table (error %d).\n", status);
    }
    history->count -= sqlite3_changes(history->storage);
}


static ClipboardEntry* clip_history_entry_for_row(sqlite3_stmt *statement)
{
    int64_t id = sqlite3_column_int64(statement, 0);
    char *text = (char*)sqlite3_column_text(statement, 1);
    char *locked = (char*)sqlite3_column_text(statement, 2);
    int count = sqlite3_column_int(statement, 3);
    return clip_clipboard_entry_new(id, text, locked != NULL, count);
}

GList* clip_history_get_list(ClipboardHistory *history)
{
    GList *list = NULL;
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_SELECT_ALL, -1, &statement, NULL);
    if(status != SQLITE_OK){
        warn("Cannot prepare history selection query (error %d).\n", status);
    } else {
        while(sqlite3_step(statement) == SQLITE_ROW){
            list = g_list_prepend(list, clip_history_entry_for_row(statement));
        }
    }
    sqlite3_finalize(statement);
    return list;
}

ClipboardEntry* clip_history_get_head(ClipboardHistory *history)
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

ClipboardEntry* clip_history_get_similar(ClipboardHistory *history, ClipboardEntry *entry, int limit_scan)
{
    if(limit_scan < 1){
        return NULL;
    }

    GList *list = clip_history_get_list(history);
    GList *next = g_list_first(list);
    char *left = clip_clipboard_entry_get_text(entry);

    int i = 0;
    ClipboardEntry *matching = NULL;
    while(i < limit_scan && next){
        ClipboardEntry *next_entry = next->data;
        if(clip_clipboard_entry_equals(entry, next_entry)){
            goto next;
        }

        char *right = clip_clipboard_entry_get_text(next_entry);
        int distance = levenshtein_distance(left, right);
        if(distance < SIMILARITY_THRESHOLD){
            trace("Distance of [%s] and [%s] is %d.\n", left, right, distance);
            matching = clip_clipboard_entry_clone(next_entry);
            break;
        }
next:
        i++;
        next = g_list_next(next);
    }
    trace("Done scanning for similarities.\n");
    clip_history_free_list(list);
    return matching;
}



static ClipboardEntry* clip_history_get_by_text(ClipboardHistory *history, char *text)
{
    ClipboardEntry *entry = NULL;
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare(history->storage, HISTORY_SELECT_BY_TEXT, -1, &statement, NULL);
    if(status != SQLITE_OK){
        warn("Cannot prepare query for text (error %d).\n", status);
    } else {
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        if(sqlite3_step(statement) == SQLITE_ROW){
            entry = clip_history_entry_for_row(statement);
        }
    }
    sqlite3_finalize(statement);
    return entry;
}

/** 
 * No author specified. See http://rosettacode.org/wiki/Levenshtein_distance#C.
 */
static int levenshtein_distance(const char *s, const char *t)
{
    int ls = strlen(s);
    int lt = strlen(t);
    int d[ls + 1][lt + 1];
    for (int i = 0; i <= ls; i++){
        for (int j = 0; j <= lt; j++){
            d[i][j] = -1;
        }
    }

    int dist(int i, int j)
    {
        if (d[i][j] >= 0){
            return d[i][j];
        }

        int x;
        if (i == ls){
            x = lt - j;
        } else if(j == lt){
            x = ls - i;
        } else if (s[i] == t[j]){
            x = dist(i + 1, j + 1);
        } else {
            x = dist(i + 1, j + 1);
            int y;
            if ((y = dist(i, j + 1)) < x){
                x = y;
            }
            if ((y = dist(i + 1, j)) < x){
                x = y;
            }
            x++;
        }
        return d[i][j] = x;
    }
    return dist(0, 0);
}
