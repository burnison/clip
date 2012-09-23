#include "gui.h"

#include "config.h"
#include "keybinder.h"
#include "utils.h"

#include <gtk/gtk.h>

static GtkMenu* menu;
//static GtkAccelGroup* accel;

static void clip_gui_hotkey_handler(const char* keystring, void* data)
{
    debug("Global hotkey pressed. Showing dialog.\n");
    gtk_main_quit();
}

void clip_gui_show()
{
    if(menu == NULL){
        debug("GUI has not yet been initialized! Will return.\n");
        return;
    }
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

void clip_gui_init()
{
    debug("Creating new GUI menu.\n");

    menu = (GtkMenu*)gtk_menu_new();

    keybinder_init();
    keybinder_bind(GLOBAL_KEY, clip_gui_hotkey_handler, NULL);
}

void clip_gui_destroy()
{
    keybinder_unbind(GLOBAL_KEY, clip_gui_hotkey_handler);

    g_free(menu);
}
