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
#include "keybinder.h"

#include "config.h"
#include "clipboard_events.h"
#include "utils.h"

#include <gtk/gtk.h>

// Not owned.
static Clipboard *clipboard;

// Owned.
static GtkWidget *menu;
static GtkWidget *menu_item_search;
static GtkWidget *menu_item_clear;
static GtkWidget *menu_item_history;
static GtkWidget *menu_item_empty;
static GtkWidget *menu_item_trim;
#ifdef DEBUG
static GtkWidget *menu_item_exit;
#endif
static gboolean marking;
static gboolean finding;
static int rows;

typedef struct {
    ClipboardEntry *entry;
    int row;
} Data;



static gboolean clip_gui_menu_item_is_selectable(GtkWidget *widget)
{
    GtkWidget *contents = gtk_bin_get_child(GTK_BIN(widget));
    return GTK_IS_LABEL(contents) && gtk_widget_is_sensitive(contents)
        && contents != gtk_bin_get_child(GTK_BIN(menu_item_clear))
        && contents != gtk_bin_get_child(GTK_BIN(menu_item_history))
        && contents != gtk_bin_get_child(GTK_BIN(menu_item_empty))
        && contents != gtk_bin_get_child(GTK_BIN(menu_item_trim))
    ;
}

static GtkWidget* clip_gui_get_selected_item(void)
{
    return gtk_menu_shell_get_selected_item(GTK_MENU_SHELL(menu));
}

static Data* clip_gui_get_data(GtkWidget *menu_item)
{
    if(menu_item == NULL || !clip_gui_menu_item_is_selectable(menu_item)){ return NULL; }
    return g_object_get_data(G_OBJECT(menu_item), "data");
}

static GtkWidget* clip_gui_menu_item_find(GCompareFunc compare)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu));
    GList *found_child = g_list_find_custom(children, NULL, compare);
    GtkWidget *found = found_child == NULL ? NULL : found_child->data;
    g_list_free(children);
    return found;
}

static GtkWidget* clip_gui_menu_item_find_for_entry(ClipboardEntry *entry)
{
    gint compare(GtkWidget *widget) {
        Data *data = clip_gui_get_data(widget);
        if(data == NULL){
            return -1;
        }
        return !clip_clipboard_entry_equals(entry, data->entry);
    }
    return clip_gui_menu_item_find((GCompareFunc)compare);
}

static void clip_gui_activate_menu_item(GtkWidget *widget)
{
    if(widget == NULL){
        debug("No widget to select.\n");
        return;
    }
    gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), widget);
    gtk_menu_item_activate(GTK_MENU_ITEM(widget));
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));
}

static ClipboardEntry* clip_gui_get_entry(GtkWidget *menu_item)
{
    Data *data = clip_gui_get_data(menu_item);
    if(data == NULL){
        return NULL;
    }
    return data->entry;
}

static ClipboardEntry* clip_gui_get_entry_copy(GtkWidget *menu_item)
{
    ClipboardEntry *entry = clip_gui_get_entry(menu_item);
    if(entry == NULL){
        return NULL;
    }
    return clip_clipboard_entry_clone(entry);
}



static void clip_gui_set_normal_label(GtkBin *item, char *text)
{
    gtk_label_set_label(GTK_LABEL(gtk_bin_get_child(item)), text);
}

static void clip_gui_set_markedup_label(GtkBin *item, char *format, char *text)
{
    GtkLabel *label = GTK_LABEL(gtk_bin_get_child(item));
    char *markedup = g_markup_printf_escaped(format, text);
    gtk_label_set_markup(label, markedup);
    g_free(markedup);
}

static void clip_gui_menu_item_update(GtkWidget *menu_item)
{
    Data *data = clip_gui_get_data(menu_item);
    if(data == NULL){
        warn("Got an update action for a missing entry.\n");
        return;
    }

    char *text = clip_clipboard_entry_get_text(data->entry);
    char *shortened = g_strndup(text, GUI_DISPLAY_CHARACTERS);
    GString *mask = g_string_new("%s");

    if(clip_clipboard_entry_get_locked(data->entry)){
        g_string_prepend(mask, "<b>");
        g_string_append(mask, "</b>");
    }

    if(clip_clipboard_is_head(clipboard, data->entry)){
        g_string_prepend(mask, "<i>");
        g_string_append(mask, "</i>");
    }

    char tag = clip_clipboard_entry_get_tag(data->entry);
    if(tag != 0){
        GString *tagged = g_string_new("");
        g_string_printf(tagged, "<span foreground='gray'>%c</span> %s", tag, mask->str);
        g_string_free(mask, TRUE);
        mask = tagged;
    }

    if(data->row < 10){
        GString *numbered = g_string_new("");
        g_string_printf(numbered, "<span foreground='gray'>%d</span> %s", data->row, mask->str);
        g_string_free(mask, TRUE);
        mask = numbered;
    }

    clip_gui_set_markedup_label(GTK_BIN(menu_item), mask->str, shortened);

    g_string_free(mask, TRUE);
    g_free(shortened);
}

static void clip_gui_menu_item_change_entry(GtkWidget *menu_item, ClipboardEntry *entry)
{
    Data *data = clip_gui_get_data(menu_item);
    if(data == NULL){
        warn("Got an update action for a missing entry.\n");
    }

    ClipboardEntry *old_entry = data->entry;
    data->entry = clip_clipboard_entry_clone(entry);
    clip_clipboard_entry_free(old_entry);

    clip_gui_menu_item_update(menu_item);
}


static void clip_gui_menu_update(void)
{
    char *search_text = clip_gui_search_in_progress()
        ? clip_gui_search_get_term()
        : GUI_SEARCH_MESSAGE;
    clip_gui_set_markedup_label(GTK_BIN(menu_item_search), "<i><b>%s</b></i>", search_text);

    char *history_text = clip_clipboard_is_enabled(clipboard)
        ? GUI_HISTORY_DISABLE_MESSAGE
        : GUI_HISTORY_ENABLE_MESSAGE;
    clip_gui_set_normal_label(GTK_BIN(menu_item_history), history_text);


    char *mode_name = GUI_AUTO_TRIM_MESSAGE;
    switch(clip_clipboard_get_trim_mode(clipboard)){
        case TRIM_OFF:
        case TRIM_STOP:  mode_name = GUI_AUTO_TRIM_MESSAGE " (Off)"; break;
        case TRIM_CHOMP: mode_name = GUI_AUTO_TRIM_MESSAGE " (Trim Right)"; break;
        case TRIM_CHUG:  mode_name = GUI_AUTO_TRIM_MESSAGE " (Trim Left)"; break;
        case TRIM_STRIP: mode_name = GUI_AUTO_TRIM_MESSAGE " (Trim Both)"; break;
    }
    clip_gui_set_normal_label(GTK_BIN(menu_item_trim), mode_name);
}





/**
 * Remove the specified menu item.
 */
static void clip_gui_menu_item_remove(GtkWidget *menu_item)
{
    Data *data = clip_gui_get_data(menu_item);
    if(data != NULL){
        g_object_set_data(G_OBJECT(menu_item),"data", NULL);
        clip_clipboard_entry_free(data->entry);
        data->entry = NULL;
        data->row = 0;
        g_free(data);
    }
    gtk_container_remove(GTK_CONTAINER(menu), menu_item);
}

static void clip_gui_do_delete(GtkWidget *menu_item)
{
    GtkWidget *selected_item = clip_gui_get_selected_item();
    if(selected_item == NULL){
        warn("Trying to remove a selected item, but no item is selected.\n");
        return;
    }

    //FIXME: I don't belong here.
    ClipboardEntry *entry = clip_gui_get_entry_copy(selected_item);
    if(!clip_clipboard_entry_get_locked(entry)){
        clip_clipboard_remove(clipboard, entry);
    } else {
        debug("Refusing to delete locked item.\n");
    }
    clip_clipboard_entry_free(entry);
}

static void clip_gui_do_lock(GtkWidget *selected_menu_item)
{
    if(selected_menu_item == NULL){
        trace("Tried to lock with no item selected.\n");
        return;
    }
    ClipboardEntry *entry = clip_gui_get_entry_copy(selected_menu_item);
    clip_clipboard_toggle_lock(clipboard, entry);
    clip_clipboard_entry_free(entry);
}

static void clip_gui_do_edit(GtkWidget *selected_menu_item, gboolean promote)
{
    if(!clip_clipboard_is_enabled(clipboard) || selected_menu_item == NULL){
        trace("Clipboard is disabled or no item is selected.\n");
        return;
    }

    trace("Editing current value.\n");
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));

    ClipboardEntry *selected_entry = clip_gui_get_entry_copy(selected_menu_item);
    clip_clipboard_disable_history(clipboard);
    char *current = clip_clipboard_entry_get_text(selected_entry);
    char *edited = clip_gui_editor_edit_text(current);
    clip_clipboard_enable_history(clipboard);

    if(edited != NULL){
        clip_clipboard_entry_set_text(selected_entry, edited);
        if(promote){
            clip_clipboard_set(clipboard, selected_entry, TRUE);
        } else {
            clip_clipboard_replace(clipboard, selected_entry);
        }
        clip_gui_menu_update();
    } else {
        debug("Edited text is unchanged. Ignoring edit request.\n");
    }
    clip_gui_editor_free_text(edited);
    clip_clipboard_entry_free(selected_entry);
}

static void clip_gui_do_join(GtkWidget *selected_menu_item)
{
    if(selected_menu_item == NULL){
        trace("Tried to join with no item selected.\n");
        return;
    }
    ClipboardEntry *entry = clip_gui_get_entry_copy(selected_menu_item);
    clip_clipboard_join(clipboard, entry);
    clip_clipboard_entry_free(entry);
}

static void clip_gui_do_change_case(GtkWidget *selected_menu_item, gboolean to_upper)
{
    if(selected_menu_item == NULL){
        trace("Tried to change case with no item selected.\n");
        return;
    }

    gboolean (*func)() = to_upper ? clip_clipboard_to_upper : clip_clipboard_to_lower;
    ClipboardEntry *entry = clip_gui_get_entry_copy(selected_menu_item);
    func(clipboard, entry);
    clip_clipboard_entry_free(entry);
}

static void clip_gui_trim(GtkWidget *selected_menu_item)
{
    if(selected_menu_item == NULL){
        trace("Tried to trim with no item selected.");
        return;
    }
    ClipboardEntry *entry = clip_gui_get_entry_copy(selected_menu_item);
    clip_clipboard_trim(clipboard, entry);
    clip_clipboard_entry_free(entry);
}

static gboolean clip_gui_do_mark(GtkWidget *selected_menu_item, guint keyval)
{
    if(selected_menu_item == NULL) { return FALSE; }
    if(keyval == GDK_KEY_space || g_unichar_iscntrl(gdk_keyval_to_unicode(keyval))) { return TRUE; }

    debug("Entering mark mode. Next letter marks.\n");
    ClipboardEntry *selected_entry = clip_gui_get_entry_copy(selected_menu_item);
    clip_clipboard_tag(clipboard, selected_entry, keyval);
    clip_clipboard_entry_free(selected_entry);
    return FALSE;
}

static gboolean clip_gui_activate_mark(guint keyval)
{
    // Control character may modify the next val. Let it pass through.
    if(keyval == GDK_KEY_space || g_unichar_iscntrl(gdk_keyval_to_unicode(keyval))) { return TRUE; }
    gint compare(GtkWidget *widget) {
        Data *data = clip_gui_get_data(widget);
        if(data == NULL) {
            return -1;
        }
        return !clip_clipboard_entry_has_tag(data->entry, keyval);
    }

    debug("Looking for mark, %c.\n", keyval);
    GtkWidget *marked = clip_gui_menu_item_find((GCompareFunc)compare);
    clip_gui_activate_menu_item(marked);
    return FALSE;
}

static void clip_gui_activate_index(unsigned int index)
{
    gint compare(GtkWidget *widget) {
        Data *data = clip_gui_get_data(widget);
        if(data == NULL) {
            return -1;
        }
        return data->row != index;
    }
    debug("Looking for index, %d.\n", index);
    GtkWidget *nth = clip_gui_menu_item_find((GCompareFunc)compare);
    clip_gui_activate_menu_item(nth);
}


static void clip_gui_select_search_match(gboolean reset_on_end)
{
    if(!clip_gui_search_in_progress() || clip_gui_search_get_length() < 1){ return; }
    gint compare(GtkWidget *widget) {
        Data *data = clip_gui_get_data(widget);
        if(data == NULL || data->row < clip_gui_search_get_position()) {
            return -1;
        }
        const char *text = clip_clipboard_entry_get_text(data->entry);
        return !g_regex_match_simple(clip_gui_search_get_term(), text, G_REGEX_CASELESS, FALSE);
    }

    GtkWidget *matching = clip_gui_menu_item_find((GCompareFunc)compare);
    if(matching != NULL) {
        Data *data = clip_gui_get_data(matching);
        gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), matching);
        clip_gui_search_set_position(data->row);
    } else {
        gtk_menu_shell_deselect(GTK_MENU_SHELL(menu));
        clip_gui_search_set_position(0);
        if(reset_on_end) {
            clip_gui_select_search_match(FALSE);
        }
    }
}

static gboolean clip_gui_cb_keypress(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    guint keyval = ((GdkEventKey*)event)->keyval;
    gboolean update_required = TRUE;

    GtkWidget *selected_menu_item = clip_gui_get_selected_item();
    if(!clip_gui_search_in_progress() && !marking && !finding){
        switch(keyval){
            case GUI_SEARCH_LEADER:
                clip_gui_search_start();
                break;
            case GDK_KEY_a:
                clip_clipboard_next_trim_mode(clipboard);
                break;
            case GDK_KEY_c:
                clip_clipboard_clear(clipboard);
                update_required = FALSE;
                break;
            case GDK_KEY_d:
            case GDK_KEY_Delete:
                clip_gui_do_delete(selected_menu_item);
                break;
            case GDK_KEY_0: case GDK_KEY_1: case GDK_KEY_2: case GDK_KEY_3: case GDK_KEY_4:
            case GDK_KEY_5: case GDK_KEY_6: case GDK_KEY_7: case GDK_KEY_8: case GDK_KEY_9:
                clip_gui_activate_index(keyval - GDK_KEY_0);
                update_required = FALSE;
                break;
            case GDK_KEY_e:
            case GDK_KEY_E:
                clip_gui_do_edit(selected_menu_item, keyval == GDK_KEY_E);
                update_required = FALSE;
                break;
            case GDK_KEY_h:
                clip_clipboard_toggle_history(clipboard);
                break;
            case GDK_KEY_J:
                clip_gui_do_join(selected_menu_item);
                update_required = FALSE;
                break;
            case GDK_KEY_L:
                clip_gui_do_lock(selected_menu_item);
                break;
            case GDK_KEY_u:
            case GDK_KEY_U:
                clip_gui_do_change_case(selected_menu_item, keyval == GDK_KEY_U);
                break;
            case GDK_KEY_t:
                clip_gui_trim(selected_menu_item);
                break;
            case GDK_KEY_m:
                marking = TRUE;
                break;
            case GDK_KEY_grave:
                finding = TRUE;
                update_required = FALSE;
                break;
            default:
                break;
        }
    } else if(marking) {
        marking = clip_gui_do_mark(selected_menu_item, keyval);

    } else if (finding) {
        finding = clip_gui_activate_mark(keyval);
        update_required = FALSE;

    } else if(clip_gui_search_in_progress()){ 
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
                clip_gui_search_increment_position();
                break;
            default:
                clip_gui_search_append(keyval);
                break;
        }
        clip_gui_select_search_match(TRUE);
    }

    if(update_required) {
        clip_gui_menu_update();
    }

    // If search mode is turned on, drop all subsequent callbacks (like mnemonics).
    return clip_gui_search_in_progress();
}

static gboolean clip_gui_cb_toggle_lock(GtkWidget *widget, GdkEvent *event, Data *data)
{
    // Only handle right button press.
    if(((GdkEventButton*)event)->button == 3) {
        clip_gui_do_lock(widget);
        return TRUE;
    }
    return FALSE;
}

/**
 * Invoked when a historical menu item has been selected.
 */
static gboolean clip_gui_cb_history_activated(GtkMenuItem *widget, Data *data)
{
    trace("History item activated.\n");
    clip_clipboard_set(clipboard, data->entry, TRUE);
    return FALSE;
}


static gboolean clip_gui_cb_clear_clipboard(GtkMenuItem *item, Data *data)
{
    trace("Clearing clipboard history.\n");
    clip_clipboard_clear(clipboard);
    return FALSE;
}


static gboolean clip_gui_cb_toggle_clipboard(GtkMenuItem *item, Data *data)
{
    trace("Toggle clipboard history.\n");
    clip_clipboard_toggle_history(clipboard);
    return FALSE;
}

/**
 * Triggered by keybinder library.
 */
static void clip_gui_cb_hotkey_handler(const char *keystring, gpointer user_data)
{
    trace("Global hotkey pressed. Showing dialog.\n");
    clip_gui_show();
}

/**
 * Removes a menu item from the menu.
 */
static void clip_gui_cb_remove_menu_item(GtkWidget *item, gpointer user_data)
{
    clip_gui_menu_item_remove(item);
}

static void clip_gui_cb_trim(GtkWidget *item, gpointer user_data)
{
    clip_clipboard_next_trim_mode(clipboard);
    clip_gui_menu_update();
}

static void clip_gui_do_add_entry(ClipboardEntry *entry, int *row)
{
    Data *data = g_malloc(sizeof(Data));
    data->entry = clip_clipboard_entry_clone(entry);
    data->row = (*row)++;

    GtkWidget *item = gtk_menu_item_new_with_label(NULL);
    g_object_set_data(G_OBJECT(item),"data", data);
    clip_gui_menu_item_update(item);

    g_signal_connect(G_OBJECT(item), "button-release-event", G_CALLBACK(clip_gui_cb_toggle_lock), data);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_history_activated), data);

    GtkLabel *label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    gtk_label_set_single_line_mode(label, TRUE);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}


/**
 * Updates the menu to contain up-to-date history entries.
 */
static void clip_gui_prepare_menu(void)
{
    trace("Preparing menu.\n");

    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)clip_gui_cb_remove_menu_item, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_search);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GList* history = clip_clipboard_get_history(clipboard);
    rows = 0;
    if(history == NULL){
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_empty);
    } else {
        g_list_foreach(history, (GFunc)clip_gui_do_add_entry, &rows);
    }
    clip_clipboard_free_history(history);
    debug("Showing %d entries.\n", rows);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_trim);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_history);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_clear);

#ifdef DEBUG
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_exit);
#endif

    clip_gui_menu_update();
    gtk_widget_show_all(menu);
}


void clip_gui_show(void)
{
    trace("Showing history menu.\n");
    if(menu == NULL){
        warn("GUI has not yet been initialized!\n");
        return;
    }
    clip_gui_prepare_menu();
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


static void clip_gui_on_event(ClipboardEvent event, ClipboardEntry* entry)
{
    switch(event){
        case CLIPBOARD_REMOVE_EVENT:
            debug("Entry removed.\n");
            clip_gui_menu_item_remove(clip_gui_menu_item_find_for_entry(entry));
            break;

        case CLIPBOARD_UPDATE_EVENT:
            debug("Entry updated.\n");
            clip_gui_menu_item_change_entry(clip_gui_menu_item_find_for_entry(entry), entry);
            break;

        case CLIPBOARD_ADD_EVENT:
        case CLIPBOARD_CLEAR_EVENT:
            debug("Clipboard cleared.\n");
            clip_gui_prepare_menu();
            break;
    }

}


void clip_gui_init(Clipboard *_clipboard)
{
    trace("Creating new GUI menu.\n");

    clip_events_add_observer(clip_gui_on_event);
    clipboard = _clipboard;

    menu = gtk_menu_new();
    gtk_widget_set_double_buffered(menu, TRUE);
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

    menu_item_trim = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_AUTO_TRIM_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_trim), "activate", G_CALLBACK(clip_gui_cb_trim), NULL);

#ifdef DEBUG
    menu_item_exit = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_DEBUG_EXIT_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_exit), "activate", G_CALLBACK(gtk_main_quit), NULL);
#endif

    keybinder_init();
    keybinder_bind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler, NULL);

    clip_gui_menu_update();
}

void clip_gui_destroy(void)
{
    keybinder_unbind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler);

    clip_gui_search_end();

    clipboard = NULL;

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

    g_object_unref(menu_item_trim);
    gtk_widget_destroy(menu_item_trim);
    menu_item_trim = NULL;

#ifdef DEBUG
    g_object_unref(menu_item_exit);
    gtk_widget_destroy(menu_item_exit);
    menu_item_exit = NULL;
#endif

    gtk_widget_destroy(menu);
    menu = NULL;
}

