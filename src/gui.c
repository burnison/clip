#include "gui.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

// Not owned.
static Clipboard* clipboard;

// Owned.
static GtkMenu* menu;


static void clip_gui_hotkey_handler(const char* keystring, void* user_data)
{
    trace("Global hotkey pressed. Showing dialog.\n");
    clip_gui_show();
}

static void clip_gui_history_selected(GtkWidget* widget)
{
    char* label = gtk_menu_item_get_label((GtkMenuItem*)widget);
    clip_clipboard_set_active(clipboard, label);
}

static void clip_gui_add_menu_item(char* text)
{
    char* copy = g_strdup(text);

    GtkWidget* item = gtk_menu_item_new_with_label(copy);
    g_signal_connect((GObject*)item, "activate", (GCallback)clip_gui_history_selected, NULL);

    GtkLabel* label = (GtkLabel*)gtk_bin_get_child((GtkBin*)item);
    gtk_label_set_single_line_mode(label, TRUE);
    gtk_label_set_max_width_chars(label, MAX_MENU_LENGTH);

    gtk_menu_shell_append((GtkMenuShell*)menu, item);
}

static void clip_gui_remove_menu_item(GtkWidget* item)
{
    gtk_container_remove((GtkContainer*)menu, item);
}

static void clip_gui_sync_menu()
{
    // Remove everything. Inefficient, but good enouh for now.
    gtk_container_foreach((GtkContainer*)menu, (GtkCallback)clip_gui_remove_menu_item, NULL);


    // Add all the new values.
    GList* history = clip_clipboard_get_history(clipboard);

    g_list_foreach(history, (GFunc)clip_gui_add_menu_item, NULL); 

    clip_clipboard_free_history(history);
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
    keybinder_bind(GLOBAL_KEY, clip_gui_hotkey_handler, NULL);
}

void clip_gui_destroy()
{
    keybinder_unbind(GLOBAL_KEY, clip_gui_hotkey_handler);

    clipboard = NULL;
    g_free(menu);
}
