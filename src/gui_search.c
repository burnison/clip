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

#include "gui_search.h"
#include "utils.h"

#include <gtk/gtk.h>


static GString* search_term;
static int position;

/**
 * Identifies if a search is currenty in progress.
 */
gboolean clip_gui_search_in_progress(void)
{
    return search_term != NULL;
}

/**
 * Terminates the search mode.
 */
void clip_gui_search_end(void)
{
    if(!clip_gui_search_in_progress()){
        return;
    }

    trace("Ending search.\n");
    if(search_term != NULL){
        g_string_free(search_term, TRUE);
        search_term = NULL;
        position = 0;
    }
}

/**
 * Begins the search mode. If already in search mode, simply pass through.
 */
void clip_gui_search_start(void)
{
    if(clip_gui_search_in_progress()){
        return;
    }

    trace("Starting new search.\n");
    search_term = g_string_new(NULL);
    position = 0;
}




/**
 * Returns the current search term string. This value must not be modified.
 */
char* clip_gui_search_get_term(void)
{
    return search_term->str;
}

int clip_gui_search_get_length(void)
{
    return search_term->len;
}

int clip_gui_search_get_position(void)
{
    return position;
}

int clip_gui_search_get_and_increment_position(void)
{
    position++;
    return clip_gui_search_get_position();
}

void clip_gui_search_set_position(int position_)
{
    position = position_;
}



/**
 * Removes a character from the search buffer. If the buffer becomes empty, search mode is terminated.
 */
void clip_gui_search_remove_char(void)
{
    if(!clip_gui_search_in_progress()){
        warn("Attempted to remove a character when search mode not enabled.\n");
        return;
    }

    if(search_term->len > 0){
        g_string_set_size(search_term, search_term->len  - 1);
    } else {
        clip_gui_search_end();
    }
}

/**
 * Append a value onto the search buffer. This function also handles starting and stopping the search mode. If keyval is
 * the search leader (i.e. the value that starts search mode), this function enters search mode and drops the character.
 * <br />
 * This function also uses the backspace character as a signal to remove on character off the buffer. When the buffer's
 * size becomes zero, the search mode is terminated.
 * <br />
 * A tab character will attempt to select the next matching value.
 * <br />
 * Control characters are dropped.
 *
 * @param keyval a GDK key value.
 */
void clip_gui_search_append(guint keyval)
{
    if(!clip_gui_search_in_progress()){
        debug("Tried to append characters when not in search mode.\n");
        return;
    } else if(!g_unichar_validate(keyval)){
        debug("Tried to append a non-unicode character to the search term.\n");
        return;
    } 

    gunichar c = gdk_keyval_to_unicode(keyval);
    if(g_unichar_iscntrl(c)) {
        debug("Control character detected. Dropping.\n");
        return;
    }

    g_string_append_unichar(search_term, c);
    position = 0;
    trace("Searching for, '%s'.\n", search_term->str);
}