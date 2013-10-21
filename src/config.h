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

#define PROGRAM "Clip"
#define CLIP_HOME ".clip"

#define HISTORY_FILE "history.sqlite"

/**
 * The number of milliseconds between rescanning the clipboards for changes.
 */
#define DAEMON_REFRESH_INTERVAL 500

/**
 * If true, Clip will sync X11's primary and selection clipboards. While
 * this is extremely useful, it results in a lot of noise.  Having used
 * this feature for over 3 years, I've finally reverted to using a more
 * convenient key mapping.
 */
#define SYNC_SELECTION 0

/**
 * The default auto-trim operation. This uses GLIB trim modes.
 */
#define DEFAULT_TRIM_MODE TRIM_CHOMP

/**
 * The maximum number of elements to retain in the history.
 */
#define HISTORY_MAX_SIZE 150

/**
 * Up to this many records will  be checked for similarity-based replacement
 * before giving up. This number should be big enough that it'll pick-up 
 * accidental selection resizing but small enough that it won't interfere
 * with older data. Setting to 0 will effectively disable this feature.
 */
#define SIMILARITY_REPLACEMENT_LIMIT 15

/**
 * The key to press to enter search mode.
 */
#define GUI_SEARCH_LEADER '/'

/**
 * The key sequence used to pop-up the clipboard menu.
 */
#define GUI_GLOBAL_KEY "<Ctrl><Alt>P"

/**
 * The maximum number of characters to display in the pop-up menu.
 */
#define GUI_DISPLAY_CHARACTERS 120

#define GUI_SEARCH_MESSAGE "Press / to search"
#define GUI_EMPTY_MESSAGE "--Clipboard Empty--"
#define GUI_CLEAR_MESSAGE "_Clear"
#define GUI_EDIT_MESSAGE "_Edit"
#define GUI_HISTORY_ENABLE_MESSAGE "Enable _History"
#define GUI_HISTORY_DISABLE_MESSAGE "Disable _History"
#define GUI_DEBUG_EXIT_MESSAGE "E_xit"
#define GUI_AUTO_TRIM_MESSAGE "_Auto Trim"

#define LOG_TRACE 0
#define LOG_DEBUG 1
#define LOG_WARN 1
