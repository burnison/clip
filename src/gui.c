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

#include "gui.h"
#include "gui_editor.h"
#include "gui_search.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

// Not owned.
static Clipboard* clipboard;

// Owned.
static GtkWidget* menu;
static GtkWidget* menu_item_search;
static GtkWidget* menu_item_clear;
static GtkWidget* menu_item_history;
static GtkWidget* menu_item_empty;
static GtkWidget* menu_item_edit;

static GList* history;

// A pointer to the currently selected element.
static ClipboardEntry* selected_entry;


/**
 * Sets a label's text.
 */
static void clip_gui_set_normal_label(GtkBin* item, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(item));
    gtk_label_set_label(label, text);
}

/**
 * Sets the text of a label and takes care of cleaning up resources.
 */
static void clip_gui_set_markedup_label(GtkBin* item, char* format, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(item));
    char* markedup = g_markup_printf_escaped(format, text);
    gtk_label_set_markup(label, markedup);
    g_free(markedup);
}

/**
 * Sets the provided menu item's text according to its state.
 */
static void clip_gui_set_entry_text(GtkWidget* menu_item, ClipboardEntry* entry)
{
    char* text = clip_clipboard_entry_get_text(entry);
    char* shortened = g_strndup(text, GUI_DISPLAY_CHARACTERS);
    GString* mask = g_string_new("%s");

    if(clip_clipboard_entry_get_locked(entry)){
        g_string_prepend(mask, "<b>");
        g_string_append(mask, "</b>");
    }

    ClipboardEntry* current = clip_clipboard_get(clipboard);
    if(!g_strcmp0(clip_clipboard_entry_get_text(current), text)){
        g_string_prepend(mask, "<i>");
        g_string_append(mask, "</i>");
    }
    clip_clipboard_entry_free(current);

    clip_gui_set_markedup_label(GTK_BIN(menu_item), mask->str, shortened);

    g_string_free(mask, TRUE);
    g_free(shortened);
}

/**
 * Updates the utility items on the menu and applies style changes as necessary.
 */
static void clip_gui_update_menu_text(void)
{
    char* search_text = clip_gui_search_in_progress()
        ? clip_gui_search_get_term()
        : GUI_SEARCH_MESSAGE;
    clip_gui_set_markedup_label(GTK_BIN(menu_item_search), "<i><b>%s</b></i>", search_text);

    char* history_text = clip_clipboard_is_enabled(clipboard)
        ? GUI_HISTORY_DISABLE_MESSAGE
        : GUI_HISTORY_ENABLE_MESSAGE;
    clip_gui_set_normal_label(GTK_BIN(menu_item_history), history_text);
}





static gboolean clip_gui_is_selectable(GtkWidget* widget)
{
    return GTK_IS_LABEL(widget) && gtk_widget_is_sensitive(widget)
        && widget != gtk_bin_get_child(GTK_BIN(menu_item_clear))
        && widget != gtk_bin_get_child(GTK_BIN(menu_item_edit))
        && widget != gtk_bin_get_child(GTK_BIN(menu_item_history))
        && widget != gtk_bin_get_child(GTK_BIN(menu_item_empty));
}


/**
 * Remove the specified menu item.
 */
static void clip_gui_remove(GtkWidget* menu_item)
{
    gtk_container_remove(GTK_CONTAINER(menu), menu_item);
}

/**
 * Remove the currently selected men item.
 */
static void clip_gui_remove_selected(void)
{
    if(selected_entry == NULL){
        warn("Trying to remove a selected entry, but no entry is selected.\n");
        return;
    } else if(clip_clipboard_entry_get_locked(selected_entry)){
        debug("Refusing to delete locked entry.\n");
        return;
    }

    clip_clipboard_remove(clipboard, selected_entry);
    selected_entry = NULL;

    GtkWidget* selected = gtk_menu_shell_get_selected_item(GTK_MENU_SHELL(menu));
    clip_gui_remove(selected);

}

/**
 * Gets the nth selectable item, starting at 0.
 */
static GtkWidget* clip_gui_get_nth_menu_item(unsigned int n)
{
    GList* children = gtk_container_get_children(GTK_CONTAINER(menu));
    GList* next = g_list_first(children);
    GtkWidget* nth = NULL;
    int i = 0;
    while((next = g_list_next(next))){
        GtkWidget* contents = gtk_bin_get_child(GTK_BIN(next->data));
        if(clip_gui_is_selectable(contents) && i++ == n){
            nth = next->data;
        }
    }
    g_list_free(children);
    return nth;
}



/**
 * Moves the currently selected item no the nth index.
 */
static void clip_gui_select_index(unsigned int index)
{
    if(index > 8){
        error("Captured an out-of-range selection index (1-9). This is probably a key mapping bug.\n");
        return;
    }

    gtk_menu_shell_deselect(GTK_MENU_SHELL(menu));
    GtkWidget* nth = clip_gui_get_nth_menu_item(index);
    if(nth != NULL){
        gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), nth);
        gtk_menu_item_activate(GTK_MENU_ITEM(nth));
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));
    }
}


/**
 * Moves the currently selected item to the nth match on the menu.
 */
static void clip_gui_search_select_match(void)
{
    if(!clip_gui_search_in_progress()){
        // Nothing can be done if there is no search in progress.
        return;
    } else if(clip_gui_search_get_length() < 1){
        // Because this is regex, an empty string will always match.
        return;
    }

    gtk_menu_shell_deselect(GTK_MENU_SHELL(menu));

    int active_position = 0;
    GList* children = gtk_container_get_children(GTK_CONTAINER(menu));
    GList* next = g_list_first(children);

    // Iterate through each list and check if it's a match.
    GtkWidget* first_match = NULL;
    while((next = g_list_next(next))){
        GtkWidget* contents = gtk_bin_get_child(GTK_BIN(next->data));
        if(!clip_gui_is_selectable(contents)){
            continue;
        }
        GtkLabel* label = GTK_LABEL(contents);
        const char* text = gtk_label_get_text(label);

        if(g_regex_match_simple(clip_gui_search_get_term(), text, G_REGEX_CASELESS, FALSE)){
            // If this is the first match, hold onto it. We need to re-pivot when resetting search position.
            if(first_match == NULL){
                first_match = GTK_WIDGET(next->data);
            }

            // Skip the first nth records that match.
            active_position++;
            if(active_position > clip_gui_search_get_position()){
                gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), GTK_WIDGET(next->data));
                break;
            }
        }
    }

    trace("Active position is %d and search position is %d.\n", active_position, clip_gui_search_get_position());
    if(clip_gui_search_get_position() == active_position){
        clip_gui_search_set_position(0);
        if(first_match != NULL){
            debug("Resetting to 0. Setting first match as active.\n");
            gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), first_match);
        }
    }

    g_list_free(children);
}

static gboolean clip_gui_cb_keypress(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    guint keyval = ((GdkEventKey*)event)->keyval;
    gboolean update_required = TRUE;
    if(!clip_gui_search_in_progress()){
        switch(keyval){
            case GUI_SEARCH_LEADER:
                clip_gui_search_start();
                break;
            case GDK_KEY_Delete:
                clip_gui_remove_selected();
                break;
            case GDK_KEY_1: case GDK_KEY_2: case GDK_KEY_3:
            case GDK_KEY_4: case GDK_KEY_5: case GDK_KEY_6:
            case GDK_KEY_7: case GDK_KEY_8: case GDK_KEY_9:
                clip_gui_select_index(keyval - GDK_KEY_1);
                update_required = FALSE;
                break;
            case GDK_KEY_j:

            default:
                break;
        }
    } else { 
        switch(keyval){
            case GDK_KEY_Escape:
            case GDK_KEY_Down:
            case GDK_KEY_Up:
            case GDK_KEY_Page_Up:
            case GDK_KEY_Page_Down:
            case GDK_KEY_Home:
            case GDK_KEY_End:
            case GDK_KEY_Return:
                clip_gui_search_end();
                update_required = FALSE;
                break;
            case GDK_KEY_BackSpace:
                clip_gui_search_remove_char();
                break;
            case GDK_KEY_Tab:
                clip_gui_search_get_and_increment_position();
                break;
            default:
                clip_gui_search_append(keyval);
                break;
        }
        clip_gui_search_select_match();
    }

    if(update_required)
    {
        clip_gui_update_menu_text();
    }

    // If search mode is turned on, drop all subsequent callbacks (like mnemonics).
    return clip_gui_search_in_progress();
}



static gboolean clip_gui_cb_toggle_lock(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    // Only handle right button press.
    if(((GdkEventButton*)event)->button == 3) {
        clip_clipboard_toggle_lock(clipboard, data);
        clip_gui_set_entry_text(widget, data);
        return TRUE;
    }
    return FALSE;
}

/**
 * Invoked when a historical menu item has been selected.
 */
static gboolean clip_gui_cb_history_activated(GtkMenuItem* widget, gpointer data)
{
    trace("History item activated.\n");

    char* text = clip_clipboard_entry_get_text(data);
    clip_clipboard_set(clipboard, text);

    return FALSE;
}


static gboolean clip_gui_cb_history_selected(GtkMenuItem* widget, gpointer data)
{
    selected_entry = data;
    return FALSE;
}


static gboolean clip_gui_cb_clear_clipboard(GtkMenuItem* item, gpointer data)
{
    trace("Clearing clipboard history.\n");
    clip_clipboard_clear(clipboard);
    return FALSE;
}


static gboolean clip_gui_cb_toggle_clipboard(GtkMenuItem* item, gpointer data)
{
    trace("Toggle clipboard history.\n");
    gboolean enabled = clip_clipboard_toggle_history(clipboard);
    gtk_widget_set_sensitive(menu_item_edit, enabled);
    return FALSE;
}

/**
 * Triggered by keybinder library.
 */
static void clip_gui_cb_hotkey_handler(const char* keystring, void* user_data)
{
    trace("Global hotkey pressed. Showing dialog.\n");
    clip_gui_show();
}

/**
 * Removes a menu item from the menu.
 */
static void clip_gui_cb_remove_menu_item(GtkWidget* item, gpointer data)
{
    gtk_container_remove(GTK_CONTAINER(menu), item);
}

static void clip_gui_cb_edit(GtkWidget* item, gpointer data)
{
    trace("Editing current value.\n");
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));

    ClipboardEntry* current_entry = clip_clipboard_get(clipboard);
    char* current = clip_clipboard_entry_get_text(current_entry);

    clip_clipboard_disable_history(clipboard);
    char* edited = clip_gui_editor_edit_text(current);
    clip_clipboard_enable_history(clipboard);

    if(edited != NULL){
        // Remove the existing value and swap it out with the new one.
        GtkWidget* head = clip_gui_get_nth_menu_item(0);
        clip_clipboard_remove(clipboard, current_entry);
        clip_gui_remove(head);

        // Set the edited value as head.
        clip_clipboard_set(clipboard, edited);
    } else {
        debug("Edited text is unchanged. Ignoring edit request.\n");
    }

    clip_gui_editor_free_text(edited);
    clip_clipboard_entry_free(current_entry);
}




static void clip_gui_entry_add(ClipboardEntry* entry)
{
    GtkWidget* item = gtk_menu_item_new_with_label(NULL);
    clip_gui_set_entry_text(item, entry);

    g_signal_connect(G_OBJECT(item), "button-release-event", G_CALLBACK(clip_gui_cb_toggle_lock), entry);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_history_activated), entry);
    g_signal_connect(G_OBJECT(item), "select", G_CALLBACK(clip_gui_cb_history_selected), entry);

    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    gtk_label_set_single_line_mode(label, TRUE);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}


/**
 * Updates the menu to contain up-to-date history entries.
 */
static void clip_gui_prepare_menu(void)
{
    selected_entry = NULL;

    // Remove everything. Inefficient, but good enouh for now.
    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)clip_gui_cb_remove_menu_item, NULL);

    // Top of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_search);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    if(history == NULL){
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_empty);
    } else {
        g_list_foreach(history, (GFunc)clip_gui_entry_add, NULL); 
    }

    // Bottom of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_history);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_clear);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_edit);

#ifdef DEBUG
    GtkWidget* menu_item_exit = gtk_menu_item_new_with_mnemonic(GUI_DEBUG_EXIT_MESSAGE);
    g_signal_connect(G_OBJECT(menu_item_exit), "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_exit);
#endif
}


void clip_gui_show(void)
{
    trace("Showing history menu.\n");
    if(menu == NULL){
        warn("GUI has not yet been initialized!\n");
        return;
    }

    // Release the cached history from last time and update it.
    clip_clipboard_free_history(history);
    history = clip_clipboard_get_history(clipboard);

    clip_gui_update_menu_text();
    clip_gui_prepare_menu();

    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


void clip_gui_init(Clipboard* _clipboard)
{
    trace("Creating new GUI menu.\n");

    clipboard = _clipboard;
    history = NULL;
    selected_entry = NULL;

    menu = gtk_menu_new();
    g_signal_connect(G_OBJECT(menu), "key-press-event", G_CALLBACK(clip_gui_cb_keypress), NULL);

    menu_item_search = g_object_ref(gtk_menu_item_new_with_label(GUI_SEARCH_MESSAGE));
    gtk_widget_set_sensitive(menu_item_search, FALSE);

    menu_item_clear = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_CLEAR_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_clear), "activate", G_CALLBACK(clip_gui_cb_clear_clipboard), NULL);

    menu_item_history = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_HISTORY_DISABLE_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_history), "activate", G_CALLBACK(clip_gui_cb_toggle_clipboard), NULL);

    menu_item_empty = g_object_ref(gtk_menu_item_new_with_label(GUI_EMPTY_MESSAGE));
    clip_gui_set_markedup_label(GTK_BIN(menu_item_empty), "<i>%s</i>", GUI_EMPTY_MESSAGE);
    gtk_widget_set_sensitive(menu_item_empty, FALSE);

    menu_item_edit = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_EDIT_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_edit), "activate", G_CALLBACK(clip_gui_cb_edit), NULL);

    keybinder_init();
    keybinder_bind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler, NULL);
}

void clip_gui_destroy(void)
{
    keybinder_unbind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler);

    clip_clipboard_free_history(history);
    history = NULL;

    clipboard = NULL;

    clip_gui_search_end();

    g_object_unref(menu_item_search);
    gtk_widget_destroy(menu_item_search);
    menu_item_search = NULL;

    g_object_unref(menu_item_history);
    gtk_widget_destroy(menu_item_history);
    menu_item_history = NULL;

    g_object_unref(menu_item_empty);
    gtk_widget_destroy(menu_item_empty);
    menu_item_empty = NULL;

    g_object_unref(menu_item_clear);
    gtk_widget_destroy(menu_item_clear);
    menu_item_clear = NULL;

    gtk_widget_destroy(menu);
    menu = NULL;
}
