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

#include "config.h"

#include <glib/gstdio.h>

#define LOG_PREFIX ""
#define LOG_SUFFIX __FILE__ " - "

#define trace(args ...) if(LOG_TRACE){ fprintf(stderr, LOG_PREFIX "TRACE - " LOG_SUFFIX args); }
#define debug(args ...) if(LOG_DEBUG){ fprintf(stderr, LOG_PREFIX "DEBUG - " LOG_SUFFIX args); }
#define warn(args ...) if(LOG_WARN){ fprintf(stderr, LOG_PREFIX "WARN  - " LOG_SUFFIX args); }
#define error(args ...) fprintf(stderr, LOG_PREFIX "ERROR - " LOG_SUFFIX args);

const char* clip_config_get_home_dir(void);
const char* clip_config_get_storage_file(void);

void clip_config_destroy(void);
