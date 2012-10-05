#include "gui.h"

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

static GList* history;

static GString* search_term;
static int search_pos;


/**
 * Usedby some callbacks.
 */
static void clip_gui_set_display_text(GtkWidget* menu_item, ClipboardHistoryEntry* entry);
static void clip_gui_update_menu(void);
static void clip_gui_set_normal_label(GtkBin* item, char* text);
static void clip_gui_set_markedup_label(GtkBin* item, char* format, char* text);


/**
 * Identifies if the menu is currently in search mode.
 */
static gboolean clip_gui_search_enabled(void)
{
    return search_term != NULL;
}

/**
 * Terminates the search mode.
 */
static void clip_gui_search_end(void)
{
    if(!clip_gui_search_enabled()){
        return;
    }

    trace("Ending search.\n");
    if(search_term != NULL){
        g_string_free(search_term, TRUE);
        search_term = NULL;
        search_pos = 0;
    }
}

/**
 * Begins the search mode. If already in search mode, simply pass through.
 */
static void clip_gui_search_start(void)
{
    if(clip_gui_search_enabled()){
        return;
    }

    trace("Starting new search.\n");
    search_term = g_string_new(NULL);
    search_pos = 0;
}

/**
 * Removes a character from the search buffer. If the buffer becomes empty, search mode is terminated.
 */
static void clip_gui_search_remove_char(void)
{
    if(!clip_gui_search_enabled()){
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
static void clip_gui_search_append(guint keyval)
{
    if(!clip_gui_search_enabled()){
        if(keyval == GUI_SEARCH_LEADER){
            clip_gui_search_start();
        }
        return;
    } 
    
    if(!g_unichar_validate(keyval)){
        debug("Tried to append a non-unicode character to the search term.\n");
        return;
    } 

    gunichar c = gdk_keyval_to_unicode(keyval);
    if(g_unichar_iscntrl(c)) {
        debug("Control character detected. Dropping.\n");
        return;
    }

    g_string_append_unichar(search_term, c);
    search_pos = 0;
    trace("Searching for, '%s'.\n", search_term->str);
}

/**
 * Moves the currently selected item to the nth match on the menu.
 */
static void clip_gui_search_select_match(void)
{
    // Because this is regex, an empty string will always match.
    if(search_term->len < 1){
        return;
    }
    gtk_menu_shell_deselect(GTK_MENU_SHELL(menu));

    int active_position = 0;
    GList* children = gtk_container_get_children(GTK_CONTAINER(menu));
    GList* next = g_list_first(children);

    GtkWidget* first_match = NULL;
    while((next = g_list_next(next))){
        GtkWidget* contents = gtk_bin_get_child(GTK_BIN(next->data));
        // If it's not a label, or if the label is disabled, ignore.
        if(!(GTK_IS_LABEL(contents) && gtk_widget_is_sensitive(contents))){
            continue;
        }
        GtkLabel* label = GTK_LABEL(contents);
        const char* text = gtk_label_get_text(label);

        if(g_regex_match_simple(search_term->str, text, G_REGEX_CASELESS, FALSE)){
            // If this is the first match, hold onto it. We need to re-pivot when resetting search_pos.
            if(first_match == NULL){
                first_match = GTK_WIDGET(next->data);
            }

            // Skip the first nth records that match.
            if(active_position++ == search_pos){
                gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), GTK_WIDGET(next->data));
                break;
            }
        }
    }

    debug("Active position is %d and search position is %d.\n", active_position, search_pos);
    if(search_pos == active_position){
        search_pos = 0;
        if(first_match != NULL){
            debug("Resetting to 0. Setting first match as active.\n");
            gtk_menu_shell_select_item(GTK_MENU_SHELL(menu), first_match);
        }
    }

    g_list_free(children);
}

static gboolean clip_gui_cb_search(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    guint keyval = ((GdkEventKey*)event)->keyval;
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
            break;

		case GDK_KEY_Delete:
			debug("Delete hook not yet implemented.\n");
			break;

        case GDK_KEY_BackSpace:
            clip_gui_search_remove_char();
            break;

        case GDK_KEY_Tab:
            trace("Increasing search position.\n");
            search_pos++;
            break;

        default:
            clip_gui_search_append(keyval);
            break;
    }

    clip_gui_update_menu();

    if(clip_gui_search_enabled()){
        clip_gui_search_select_match();
    }

    // If search mode is turned on, drop all subsequent callbacks (like mnemonics).
    return clip_gui_search_enabled();
}



static gboolean clip_gui_cb_toggle_sticky(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    // Only handle right button press.
    if(((GdkEventButton*)event)->button != 3) {
        return FALSE;
    }

    clip_clipboard_toggle_lock(clipboard, data);
    clip_gui_set_display_text(widget, data);

    return TRUE;
}

/**
 * Invoked when a historical menu item has been selected.
 */
static gboolean clip_gui_cb_history_selected(GtkMenuItem* widget, gpointer data)
{
    trace("History item selected.\n");

    char* text = clip_history_entry_get_text(data);
    clip_clipboard_set_active(clipboard, text);
    clip_history_entry_free_text(text);

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
    clip_clipboard_toggle(clipboard);
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






/**
 * Sets a label's text
 */
static void clip_gui_set_normal_label(GtkBin* item, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(item));
    gtk_label_set_label(label, text);
}

static void clip_gui_set_markedup_label(GtkBin* item, char* format, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(item));
    char* markedup = g_markup_printf_escaped(format, text);
    gtk_label_set_markup(label, markedup);
    g_free(markedup);
}

static void clip_gui_update_menu(void)
{
    char* search_text = clip_gui_search_enabled()
        ? search_term->str
        : GUI_SEARCH_MESSAGE;
    clip_gui_set_markedup_label(GTK_BIN(menu_item_search), "<i><b>%s</b></i>", search_text);

    char* history_text = clip_clipboard_is_enabled(clipboard)
        ? GUI_HISTORY_DISABLE_MESSAGE
        : GUI_HISTORY_ENABLE_MESSAGE;
    clip_gui_set_normal_label(GTK_BIN(menu_item_history), history_text);
}




static void clip_gui_set_display_text(GtkWidget* menu_item, ClipboardHistoryEntry* entry)
{
    char* entry_text = clip_history_entry_get_text(entry);
    char* shortened = g_strndup(entry_text, GUI_DISPLAY_CHARACTERS);

    if(clip_history_entry_get_locked(entry)){
        clip_gui_set_markedup_label(GTK_BIN(menu_item), "<b>%s</b>", shortened);
    } else {
        clip_gui_set_normal_label(GTK_BIN(menu_item), shortened);
    }

    g_free(shortened);
    clip_history_entry_free_text(entry_text);
}

static void clip_gui_add_menu_item(ClipboardHistoryEntry* entry)
{
    GtkWidget* item = gtk_menu_item_new_with_label(NULL);
    clip_gui_set_display_text(item, entry);

    g_signal_connect(G_OBJECT(item), "button-release-event", G_CALLBACK(clip_gui_cb_toggle_sticky), entry);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_history_selected), entry);


    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    gtk_label_set_single_line_mode(label, TRUE);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

}

static void clip_gui_sync_menu(void)
{
    // Remove everything. Inefficient, but good enouh for now.
    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)clip_gui_cb_remove_menu_item, NULL);

    clip_gui_update_menu();

    // Top of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_search);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Release the history from last time.
    clip_clipboard_free_history(history);
    history = clip_clipboard_get_history(clipboard);
    if(history == NULL){
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_empty);
    } else {
        g_list_foreach(history, (GFunc)clip_gui_add_menu_item, NULL); 
    }

    // Bottom of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_history);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_clear);

#ifdef DEBUG
    GtkWidget* menu_item_exit = gtk_menu_item_new_with_mnemonic(GUI_DEBUG_EXIT_MESSAGE);
    g_signal_connect(G_OBJECT(menu_item_exit), "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_exit);
#endif
}


static void clip_gui_setup_menu(void)
{
    menu = gtk_menu_new();
    g_signal_connect(G_OBJECT(menu), "key-press-event", G_CALLBACK(clip_gui_cb_search), NULL);

    menu_item_search = g_object_ref(gtk_menu_item_new_with_label(GUI_SEARCH_MESSAGE));
    gtk_widget_set_sensitive(menu_item_search, FALSE);

    menu_item_clear = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_CLEAR_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_clear), "activate", G_CALLBACK(clip_gui_cb_clear_clipboard), NULL);

    menu_item_history = g_object_ref(gtk_menu_item_new_with_mnemonic(GUI_HISTORY_DISABLE_MESSAGE));
    g_signal_connect(G_OBJECT(menu_item_history), "activate", G_CALLBACK(clip_gui_cb_toggle_clipboard), NULL);

    menu_item_empty = g_object_ref(gtk_menu_item_new_with_label(GUI_EMPTY_MESSAGE));
    clip_gui_set_markedup_label(GTK_BIN(menu_item_empty), "<i>%s</i>", GUI_EMPTY_MESSAGE);
    gtk_widget_set_sensitive(menu_item_empty, FALSE);
}



void clip_gui_show(void)
{
    trace("Showing history menu.\n");

    if(menu == NULL){
        warn("GUI has not yet been initialized!\n");
        return;
    }

    clip_gui_sync_menu();

    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


void clip_gui_init(Clipboard* _clipboard)
{
    trace("Creating new GUI menu.\n");

    clipboard = _clipboard;

    clip_gui_setup_menu();

    keybinder_init();
    keybinder_bind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler, NULL);
}

void clip_gui_destroy(void)
{
    keybinder_unbind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler);

    clip_clipboard_free_history(history);
    history = NULL;

    clipboard = NULL;

    // May be null if not in search mode.
    if(search_term != NULL){
        g_string_free(search_term, TRUE);
        search_term = NULL;
    }

    gtk_widget_destroy(menu);
    menu = NULL;
}
