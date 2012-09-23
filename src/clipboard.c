#include "clipboard.h"
#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <string.h>


struct clipboard {
    GtkClipboard* provider;
    char* text;
};


Clipboard* clip_new_clipboard(void* provider)
{
    Clipboard* clipboard = g_malloc(sizeof(Clipboard));
    clipboard->provider = provider;
    clipboard->text = g_malloc(0);

    return clipboard;
}

void clip_free_clipboard(Clipboard* clipboard)
{
    if(clipboard == NULL){
        return;
    }

    // Managed by GTK.
    clipboard->provider = NULL;
    g_free(clipboard->text);
    g_free(clipboard);
}

void* clip_get_provider(Clipboard* clipboard)
{
    return clipboard->provider;
}

char* clip_get_text(Clipboard* clipboard)
{
    return clipboard->text;
}

void clip_set_text(Clipboard* clipboard, char* text)
{
    int length = 0;
    if(g_utf8_validate(text, -1, NULL)){
        length =  g_utf8_strlen(text, -1);
    } else {
        length = strlen(text);
    }
    
    if(clipboard->text != NULL){
        g_free(clipboard->text);
    }
    clipboard->text = g_malloc(length + 1);

    g_strlcpy(clipboard->text, text, length + 1);
}
