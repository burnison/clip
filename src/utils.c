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

#include "utils.h"

static char *home;
static char *storage;

// Freed by destroy.
const char* clip_config_get_home_dir(void)
{
    if(home == NULL){
        home = g_build_path("/", g_get_home_dir(), CLIP_HOME, NULL);
    }
    return home;
}

// Freed by destroy.
const char* clip_config_get_storage_file(void)
{
    if(storage == NULL){
        storage = g_build_path("/", clip_config_get_home_dir(), HISTORY_FILE, NULL);
    }
    return storage;
}

void clip_config_destroy(void)
{
    g_free(home);
    home = NULL;

    g_free(storage);
    storage = NULL;
}
