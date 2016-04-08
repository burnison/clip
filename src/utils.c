/*
 * Copyright (c) 2016 Richard Burnison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
