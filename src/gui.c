#include "gui.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

// Not owned.
static Clipboard* clipboard;

// Owned.
static GtkMenu* menu;

/**
 * Triggered by keybinder library.
 */
static void clip_gui_callback_hotkey_handler(const char* keystring, void* user_data)
{
    trace("Global hotkey pressed. Showing dialog.\n");
    clip_gui_show();
}

/**
 * Removes a menu item from the menu.
 */
static void clip_gui_callback_remove_menu_item(GtkWidget* item)
{
    gtk_container_remove((GtkContainer*)menu, item);
}

/**
 * Frees the historical text (i.e. clipboard contents) used in a signal. Called when a menu item used for historical
 * values is destroy.
 */
static void clip_gui_callback_remove_signal_data(GtkWidget* widget, gpointer data)
{
    g_free(data);
}

/**
 * Invoked when a historical menu item has been selected.
 */
static void clip_gui_callback_history_selected(GtkWidget* widget, gpointer data)
{
    clip_clipboard_set_active(clipboard, data);
}

static void clip_gui_callback_clear_clipboard(void)
{
    clip_clipboard_clear(clipboard);
}

/**
 * Adds a menu item to clear the clipboard's history.
 */
static void clip_gui_add_clear_item(void)
{
    GtkWidget* item = gtk_menu_item_new_with_mnemonic("_Clear");
    g_signal_connect((GObject*)item, "activate", (GCallback)clip_gui_callback_clear_clipboard, NULL);

    gtk_menu_shell_append((GtkMenuShell*)menu, item);
}

/**
 * Adds a menu item to disable the copying.
 */
static void clip_gui_add_disable_item(void)
{
    return;
}


static void clip_gui_add_search_box(void)
{
    GtkWidget* item = gtk_menu_item_new();

    GtkWidget* textbox = gtk_entry_new();
    gtk_container_add((GtkContainer*)item, textbox);

    gtk_menu_shell_append((GtkMenuShell*)menu, item);
}


/**
 * Adds a placeholder menu item to indicate the clipboard is empty.
 */
static void clip_gui_add_empty_placeholder(void)
{
    GtkWidget* item = gtk_menu_item_new_with_label(NULL);
    gtk_widget_set_sensitive(item, FALSE);

    GtkLabel* label = (GtkLabel*)gtk_bin_get_child((GtkBin*)item);
    char* empty = g_markup_printf_escaped("<i>%s</i>", EMPTY_MESSAGE);
    gtk_label_set_markup((GtkLabel*)label, empty);
    g_free(empty);

    gtk_menu_shell_append((GtkMenuShell*)menu, item);
}

static void clip_gui_add_menu_item(char* text)
{
    // Create a shortened display text. Using a large block becomes clunky in GTK.
    char* shortened = g_strndup(text, MAX_MENU_LENGTH);
    GtkWidget* item = gtk_menu_item_new_with_label(shortened);
    g_free(shortened);

    // The copy is freed by the destroy callback.
    char* copy = g_strdup(text);
    g_signal_connect((GObject*)item, "activate", (GCallback)clip_gui_callback_history_selected, copy);
    g_signal_connect((GObject*)item, "destroy", (GCallback)clip_gui_callback_remove_signal_data, copy);

    GtkLabel* label = (GtkLabel*)gtk_bin_get_child((GtkBin*)item);
    gtk_label_set_single_line_mode(label, TRUE);

    gtk_menu_shell_append((GtkMenuShell*)menu, item);

}

static void clip_gui_sync_menu()
{
    // Remove everything. Inefficient, but good enouh for now.
    gtk_container_foreach((GtkContainer*)menu, (GtkCallback)clip_gui_callback_remove_menu_item, NULL);

    // Top of menu.
    clip_gui_add_search_box();
    gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());

    // Add the history.
    GList* history = clip_clipboard_get_history(clipboard);
    if(history == NULL){
        clip_gui_add_empty_placeholder();
    } else {
        g_list_foreach(history, (GFunc)clip_gui_add_menu_item, NULL); 
    }
    clip_clipboard_free_history(history);

    // Bottom of menu.
    gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
    clip_gui_add_clear_item();
}


void clip_gui_show()
{
    trace("Showing history menu.\n");
    if(menu == NULL){
        debug("GUI has not yet been initialized! Will return.\n");
        return;
    }
    clip_gui_sync_menu();
    gtk_widget_show_all((GtkWidget*)menu);
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


void clip_gui_init(Clipboard* _clipboard)
{
    debug("Creating new GUI menu.\n");
    
    menu = (GtkMenu*)gtk_menu_new();
    clipboard = _clipboard;

    keybinder_init();
    keybinder_bind(GLOBAL_KEY, clip_gui_callback_hotkey_handler, NULL);
}

void clip_gui_destroy()
{
    keybinder_unbind(GLOBAL_KEY, clip_gui_callback_hotkey_handler);

    clipboard = NULL;

    g_free(menu);
    menu = NULL;
}
