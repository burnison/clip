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

#include <glib.h>

void clip_gui_search_end(void);
void clip_gui_search_start(void);
gboolean clip_gui_search_in_progress(void);


void clip_gui_search_append(guint keyval);
void clip_gui_search_remove_char(void);


char* clip_gui_search_get_term(void);
int clip_gui_search_get_length(void);

int clip_gui_search_get_position(void);
void clip_gui_search_set_position(int position);
void clip_gui_search_increment_position(void);
