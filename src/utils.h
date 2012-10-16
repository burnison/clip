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
