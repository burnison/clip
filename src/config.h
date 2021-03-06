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

#define PROGRAM "Clip"
#define CLIP_HOME ".clip"

#define HISTORY_FILE "history.sqlite"

/**
 * The number of milliseconds between rescanning the clipboards for changes.
 */
#define DAEMON_REFRESH_INTERVAL 500

/**
 * If true, Clip will sync X11's primary and 'clipboard' clipboards. While
 * this is extremely useful, it results in a lot of noise. After about 3 years
 * using this feature, I've decided to disable it in favour of SYNC_PRIMARY,
 * which is quite often what I actually want. This feature is supported but
 * discouraged.
 *
 * N.B. This feature *should* not be used with SYNC_PRIMARY, as the two features
 * will start interfering each each other.
 */
#define SYNC_CLIPBOARDS 0
/**
 * If true, Clip will copy the X11 'clipboard' clipboard to the primary
 * clipboard. Unlike SYNC_CLIPBOARDS, changes to primary will not be copied to
 * 'clipboard'.
 *
 * N.B. This feature *should* not be used with SYNC_PRIMARY, as the two features
 * will start interfering each each other.
 */
#define SYNC_PRIMARY 1
#define SYNC_ANY SYNC_CLIPBOARDS||SYNC_PRIMARY

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

#define GUI_MASK_CHAR '*'

/**
 * The maximum number of characters to display in the pop-up menu.
 */
#define GUI_DISPLAY_CHARACTERS 120

#define GUI_SEARCH_MESSAGE "Press / to search"
#define GUI_EMPTY_MESSAGE "--Clipboard Empty--"
#define GUI_CLEAR_MESSAGE "Clear"
#define GUI_HISTORY_ENABLE_MESSAGE "Enable History"
#define GUI_HISTORY_DISABLE_MESSAGE "Disable History"
#define GUI_AUTO_TRIM_MESSAGE "Auto Trim"
#define GUI_DEBUG_EXIT_MESSAGE "E_xit"

#define LOG_TRACE 0
#define LOG_DEBUG 1
#define LOG_WARN 1
