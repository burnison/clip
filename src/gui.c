#include "gui.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

// Not owned.
static Clipboard* clipboard;

// Owned.
static GtkWidget* menu = NULL;
static GtkWidget* menu_item_search = NULL;
static GtkWidget* menu_item_clear = NULL;
static GtkWidget* menu_item_history = NULL;
static GtkWidget* menu_item_empty = NULL;
static GString* search_term = NULL;


/**
 * Usedby some callbacks.
 */
static void clip_gui_update_menu(void);



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
 * Frees the historical text (i.e. clipboard contents) used in a signal. Called
 * when a menu item used for historical values is destroy.
 */
static void clip_gui_cb_remove_signal_data(GtkWidget* widget, gpointer data)
{
    g_free(data);
}





static gboolean clip_gui_in_search(void)
{
    return search_term != NULL;
}

static void clip_gui_end_search(void)
{
    if(search_term != NULL){
        g_string_free(search_term, TRUE);
        search_term = NULL;
    }
}

static void clip_gui_start_search(void)
{
    if(clip_gui_in_search()){
        return;
    }
    search_term = g_string_new(NULL);
}

static void clip_gui_append_search(guint keyval)
{
    if(!clip_gui_in_search()){
        if(keyval == GUI_SEARCH_LEADER){
            debug("Starting new search.\n");
            clip_gui_start_search();
        }
        return;
    }

    if(!g_unichar_validate(keyval)){
        warn("Tried to append a non-unicode character to the search term.");

    } else if(keyval == GDK_KEY_BackSpace){
        if(search_term->len > 0){
            g_string_set_size(search_term, search_term->len  - 1);

        } else { 
            clip_gui_end_search();
            return;
        }
    }

    gunichar c = gdk_keyval_to_unicode(keyval);
    if(g_unichar_iscntrl(c)) {
        debug("Ignoring control characters.\n");
        return;
    }

    g_string_append_unichar(search_term, c);
    trace("Searching for, '%s'.\n", search_term->str);
}


static gboolean clip_gui_cb_search(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    GdkEventKey* e = ((GdkEventKey*)event);

    switch(e->keyval){
        case GDK_KEY_Escape:
        case GDK_KEY_Down:
        case GDK_KEY_Up:
        case GDK_KEY_Page_Up:
        case GDK_KEY_Page_Down:
        case GDK_KEY_Home:
        case GDK_KEY_End:
        case GDK_KEY_Return:
            clip_gui_end_search();
            break;

        // Fall through after the first occurance time.
        case GUI_SEARCH_LEADER:
        default:
            clip_gui_append_search(e->keyval);
            break;
    }

    // Force the menu to get updated.
    clip_gui_update_menu();

    // If we're in a search mode, drop all subsequent hooks..
    return clip_gui_in_search();
}


/**
 * Invoked when a historical menu item has been selected.
 */
static void clip_gui_cb_history_selected(GtkMenuItem* widget, gpointer data)
{
    clip_clipboard_set_active(clipboard, data);
}


static void clip_gui_cb_clear_clipboard(GtkMenuItem* item, gpointer data)
{
    clip_clipboard_clear(clipboard);
}


static void clip_gui_cb_disable_clipboard(GtkMenuItem* item, gpointer data)
{
    clip_clipboard_toggle(clipboard);
}



static void clip_gui_set_normal_label(GtkWidget* item, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    gtk_label_set_label(label, text);
}

static void clip_gui_set_italic_label(GtkWidget* item, char* text)
{
    GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    char* italic = g_markup_printf_escaped("<i>%s</i>", text);
    gtk_label_set_markup(label, italic);
    g_free(italic);
}

static void clip_gui_update_menu(void)
{
    char* search_text = clip_gui_in_search()
        ? search_term->str
        : GUI_SEARCH_MESSAGE;
    clip_gui_set_italic_label(menu_item_search, search_text);

    char* history_text = clip_clipboard_is_enabled(clipboard)
        ? GUI_HISTORY_DISABLE_MESSAGE
        : GUI_HISTORY_ENABLE_MESSAGE;
    clip_gui_set_normal_label(menu_item_history, history_text);
}






static void clip_gui_add_menu_item(char* text)
{
    // Create a shortened display text. Large block becomes clunky in GTK.
    char* shortened = g_strndup(text, GUI_DISPLAY_CHARACTERS);
    GtkWidget* item = gtk_menu_item_new_with_label(shortened);
    g_free(shortened);

    // The copy is freed by the destroy callback.
    char* copy = g_strdup(text);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_history_selected), copy);
    g_signal_connect(G_OBJECT(item), "destroy", G_CALLBACK(clip_gui_cb_remove_signal_data), copy);

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

    // Add the history.
    GList* history = clip_clipboard_get_history(clipboard);
    if(history == NULL){
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_empty);
    } else {
        g_list_foreach(history, (GFunc)clip_gui_add_menu_item, NULL); 
    }
    clip_clipboard_free_history(history);

    // Bottom of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_history);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_clear);
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
    g_signal_connect(G_OBJECT(menu_item_history), "activate", G_CALLBACK(clip_gui_cb_disable_clipboard), NULL);

    menu_item_empty = g_object_ref(gtk_menu_item_new_with_label(GUI_EMPTY_MESSAGE));
    clip_gui_set_italic_label(menu_item_empty, GUI_EMPTY_MESSAGE);
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

    clipboard = NULL;

    g_string_free(search_term, TRUE);
    search_term = NULL;

    gtk_widget_destroy(menu);
    menu = NULL;
}
