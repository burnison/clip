#include "gui.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

static const char* LABEL_CLEAR = "_Clear";
static const char* LABEL_ENABLE = "Enable _History";
static const char* LABEL_DISABLE = "Disable _History";

// Not owned.
static Clipboard* clipboard;

// Owned.
static GtkWidget* menu;

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

/**
 * Adds a menu item to clear the clipboard's history.
 */
static void clip_gui_add_clear_item(void)
{
    GtkWidget* item = gtk_menu_item_new_with_mnemonic(LABEL_CLEAR);

    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_clear_clipboard), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

/**
 * Adds a menu item to disable the copying.
 */
static void clip_gui_add_toggle_item(void)
{
    const char* label = clip_clipboard_is_enabled(clipboard)
            ? LABEL_DISABLE
            : LABEL_ENABLE;

    GtkWidget* item = gtk_menu_item_new_with_mnemonic(label);

    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clip_gui_cb_disable_clipboard), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}


static void clip_gui_add_search_box(void)
{
    GtkWidget* item = gtk_menu_item_new_with_label("Exit");
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(gtk_main_quit), NULL);

//    GtkWidget* textbox = gtk_entry_new();
//    gtk_container_add(GTK_CONTAINER(item), textbox);
//
//    GtkEntryCompletion* completion = gtk_entry_completion_new();
//    gtk_entry_set_completion(GTK_ENTRY(textbox), GTK_ENTRY_COMPLETION(completion));
//
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}


/**
 * Adds a placeholder menu item to indicate the clipboard is empty.
 */
static void clip_gui_add_empty_placeholder(void)
{
    GtkWidget* item = gtk_menu_item_new_with_label(NULL);
    gtk_widget_set_sensitive(item, FALSE);

    GtkWidget* label = gtk_bin_get_child(GTK_BIN(item));
    char* empty = g_markup_printf_escaped("<i>%s</i>", GUI_EMPTY_MESSAGE);
    gtk_label_set_markup(GTK_LABEL(label), empty);
    g_free(empty);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
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

    // Top of menu.
    clip_gui_add_search_box();
    clip_gui_add_toggle_item();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Add the history.
    GList* history = clip_clipboard_get_history(clipboard);
    if(history == NULL){
        clip_gui_add_empty_placeholder();
    } else {
        g_list_foreach(history, (GFunc)clip_gui_add_menu_item, NULL); 
    }
    clip_clipboard_free_history(history);

    // Bottom of menu.
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    clip_gui_add_clear_item();
}


void clip_gui_show(void)
{
    trace("Showing history menu.\n");

    if(menu == NULL){
        warn("GUI has not yet been initialized!\n");
        return;
    }
    clip_gui_sync_menu();
    gtk_widget_show_all(GTK_WIDGET(menu));
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


void clip_gui_init(Clipboard* _clipboard)
{
    trace("Creating new GUI menu.\n");

    menu = gtk_menu_new();

    clipboard = _clipboard;

    keybinder_init();
    keybinder_bind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler, NULL);
}

void clip_gui_destroy(void)
{
    keybinder_unbind(GUI_GLOBAL_KEY, clip_gui_cb_hotkey_handler);

    clipboard = NULL;

    gtk_widget_destroy(menu);
    menu = NULL;
}
