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

#include "clipboard_events.h"
#include "config.h"
#include "history.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>


#define HISTORY_CREATE "CREATE TABLE IF NOT EXISTS history(" \
                               "    id INTEGER PRIMARY KEY,"\
                               "    created TIMESTAMP NOT NULL DEFAULT current_timestamp,"\
                               "    text TEXT NOT NULL UNIQUE,"\
                               "    usage_count BIGINT NOT NULL DEFAULT 0,"\
                               "    locked INT NOT NULL DEFAULT 0,"\
                               "    tag CHAR(1) UNIQUE"\
                               ")"

#define HISTORY_INSERT_EXISTING "UPDATE history SET text = ?1, created = current_timestamp, usage_count = usage_count + 1 WHERE id = ?2"
// Rather than doing an insert+select+update or a constraint violation, just do a replace.
#define HISTORY_INSERT_NEW "INSERT OR REPLACE INTO history(id, text, created, locked, usage_count, tag) VALUES( "\
                                "(SELECT id FROM history WHERE text = ?1), "\
                                "?1, current_timestamp, "\
                                "(SELECT locked FROM history WHERE text = ?1), "\
                                "(SELECT usage_count + 1 FROM history WHERE text = ?1), "\
                                "(SELECT tag FROM history WHERE text = ?1)"\
                                ")"

#define HISTORY_UPDATE_BY_ID "UPDATE history SET "\
                                "text = ?1, "\
                                "locked = ?2, "\
                                "tag = ?3 "\
                                "WHERE id = ?4"

#define HISTORY_DELETE_UNLOCKED_BY_ID "DELETE FROM history WHERE id = ? AND locked = 0"
#define HISTORY_DELETE_UNLOCKED_BY_AGE "DELETE FROM history WHERE id = (SELECT max(id) FROM history WHERE locked = 0)"

#define HISTORY_CLEAR "DELETE FROM history WHERE locked = 0"

// Use a LRU+LFU eviction policy.
#define HISTORY_EVICT_SINGLE "DELETE FROM history WHERE id = (SELECT id FROM history WHERE locked = 0 ORDER BY usage_count, id LIMIT 1)"

#define HISTORY_SELECT_ALL "SELECT id, text, locked, usage_count, tag FROM history ORDER BY created, id"
#define HISTORY_SELECT_BY_TEXT "SELECT id, text, locked, usage_count, tag FROM history WHERE text = ?1"
#define HISTORY_SELECT_COUNT "SELECT count(*) FROM history"

static int levenshtein_distance(const char *s, const char *t);
static ClipboardEntry* clip_history_get_by_text(ClipboardHistory *history, char *text);

struct history {;
    sqlite3 *storage;
    int count;
    GList *observers;
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
    history->observers = NULL;

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
        g_list_free(history->observers);
        history->observers = NULL;
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
    if(clip_clipboard_entry_get_text(entry) == NULL){
        error("Refusing to persist a null entry.");
        return FALSE;
    }

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

    if(success){
        clip_events_notify(CLIPBOARD_ADD_EVENT, entry);
        return TRUE;
    }
    return FALSE;
}

/**
 * Issue a delete if there is another record with the same text but different ID. This could happen in the case where
 * downstream there was a replacement request (that is, an existing record, A, has been changed to B, when B is already
 * an existing record.
 */
static gboolean clip_history_remove_duplicates(ClipboardHistory *history, ClipboardEntry *entry)
{
    gboolean success = TRUE;
    int64_t id = clip_clipboard_entry_get_id(entry);
    ClipboardEntry *existing = clip_history_get_by_text(history, clip_clipboard_entry_get_text(entry));
    if(existing != NULL && !clip_clipboard_entry_same(entry, existing)){
        debug("Entry, %"PRIu64", already has that value.\n", id);
        if(!clip_history_remove(history, existing)){
            warn("Couldn't remove existing record, %"PRIu64" with desired text.\n", id);
            success = FALSE;
        }
    }
    clip_clipboard_entry_free(existing);
    return success;
}


gboolean clip_history_update(ClipboardHistory *history, ClipboardEntry *entry)
{
    char *text = clip_clipboard_entry_get_text(entry);
    if(text == NULL){
        error("Refusing to update a null entry.");
        return FALSE;
    }

    gboolean success = TRUE;
    int64_t id = clip_clipboard_entry_get_id(entry);
    if (!clip_history_remove_duplicates(history, entry)) {
        success = FALSE;
        goto exit;
    }

    sqlite3_stmt *statement = NULL;
    trace("Updating existing entry, %"PRIu64".\n", id);
    int status = sqlite3_prepare(history->storage, HISTORY_UPDATE_BY_ID, -1, &statement, NULL);
    if(status == SQLITE_OK){
        char tag = clip_clipboard_entry_get_tag(entry);
        sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 2, clip_clipboard_entry_get_locked(entry));
        sqlite3_bind_text(statement, 3, tag == 0 ? NULL : &tag, 1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 4, id);
        if((status = sqlite3_step(statement)) != SQLITE_DONE){
            warn("Couldn't update entry, %"PRIu64" (error %d).\n", id, status);
            success = FALSE;
        } else {
            clip_events_notify(CLIPBOARD_UPDATE_EVENT, entry);
        }
    } else {
        warn("Couldn't prepare entry update for %"PRIu64" (error %d).\n", id, status);
        success = FALSE;
    }
    sqlite3_finalize(statement);
exit:
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
        } else {
            clip_events_notify(CLIPBOARD_REMOVE_EVENT, entry);
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
    } else {
        clip_events_notify(CLIPBOARD_CLEAR_EVENT, NULL);
    }
    history->count -= sqlite3_changes(history->storage);
}


static ClipboardEntry* clip_history_entry_for_row(sqlite3_stmt *statement)
{
    int64_t id = sqlite3_column_int64(statement, 0);
    char *text = (char*)sqlite3_column_text(statement, 1);
    gboolean locked = sqlite3_column_int(statement, 2);
    int count = sqlite3_column_int(statement, 3);
    char *tag = (char*)sqlite3_column_text(statement, 4);
    return clip_clipboard_entry_new(id, text, locked, count, tag == NULL ? 0 : tag[0]);
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


static gboolean clip_history_levenshtein_similar(char *left, char *right) {
    int distance = levenshtein_distance(left, right);
    int shortest = MIN(strlen(left), strlen(right));
    // Magic numbers. These are an arbitrary crapshoot, anyways.
    int threshold = shortest / 100 + 3;
    if(distance < threshold){
        trace("Distance of [%s] and [%s] is %d.\n", left, right, distance);
        return TRUE;
    }
    return FALSE;
}

ClipboardEntry* clip_history_get_similar(ClipboardHistory *history, ClipboardEntry *entry, int limit_scan)
{
    if(limit_scan < 1){
        return NULL;
    }

    ClipboardEntry *matching = NULL;
    GList *list = clip_history_get_list(history);
    GList *next = g_list_first(list);
    char *left = clip_clipboard_entry_get_text(entry);
    if(left == NULL){
        warn("New entry has no text.\n");
        goto exit;
    }

    int i = 0;
    while(i < limit_scan && next != NULL){
        // If this is the same entry, skip it.
        ClipboardEntry *next_entry = next->data;
        if(clip_clipboard_entry_equals(entry, next_entry)){
            goto next;
        }

        char *right = clip_clipboard_entry_get_text(next_entry);
        if(right == NULL){
            warn("Historic entry has null text.\n");
            goto exit;
        }

        gboolean found_similar = clip_history_levenshtein_similar(left, right);
        if(found_similar){
            matching = clip_clipboard_entry_clone(next_entry);
            break;
        }
next:
        i++;
        next = g_list_next(next);
    }
    trace("Done scanning for similarities.\n");
exit:
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
    int ls = MIN(1024, strlen(s));
    int lt = MIN(1024, strlen(t));
    int *d = g_malloc((ls + 1) * (lt + 1) * sizeof(int));
    for (int i = 0; i <= ls; i++){
        for (int j = 0; j <= lt; j++){
            d[i*j+j] = -1;
        }
    }

    int dist(int i, int j)
    {
        if (d[i*j+j] >= 0){
            return d[i*j+j];
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
        return d[i*j+j] = x;
    }
    int distance = dist(0, 0);
    g_free(d);
    return distance;
}
